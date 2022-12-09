#include "scan/thread.h"
#include "db/profile_reader.h"
#include "die.h"
#include "errmsg.h"
#include "fs.h"
#include "h3c/h3c.h"
#include "hmmer/client.h"
#include "hmmer/result.h"
#include "logy.h"
#include "loop/now.h"
#include "progress.h"
#include "scan/cfg.h"
#include "scan/prot_match_iter.h"
#include "xmath.h"
#include <string.h>

void thread_init(struct thread *t, FILE *prodfile, int idx, int prof_start,
                 struct profile_reader *reader, struct scan_cfg cfg)
{
    t->prodfile = prodfile;
    t->idx = idx;
    t->prof_start = prof_start;
    t->seq = NULL;
    t->reader = reader;
    t->cfg = cfg;
    progress_init(&t->progress, 0);
    hmmer_result_new(&t->hmmer_result);
    t->errnum = RC_OK;
    t->errmsg[0] = '\0';
}

void thread_setup_job(struct thread *t, enum imm_abc_typeid abc_typeid,
                      enum profile_typeid profile_typeid, long scan_id,
                      long ntasks)
{
    char const *abc = imm_abc_typeid_name(abc_typeid);
    char const *prof = profile_typeid_name(profile_typeid);
    prod_setup_job(&t->prod, abc, prof, scan_id);
    progress_init(&t->progress, ntasks);
}

void thread_setup_seq(struct thread *t, struct imm_seq const *seq, long seq_id)
{
    t->seq = seq;
    prod_setup_seq(&t->prod, seq_id);
}

static enum rc reset_task(struct thread *t, struct imm_task **task,
                          struct imm_dp const *dp);
static enum rc setup_task(struct thread *t, struct imm_task *task,
                          struct imm_seq const *seq);
static enum rc setup_hypothesis(struct thread *t, struct hypothesis *hyp,
                                struct imm_dp const *dp,
                                struct imm_seq const *seq);
static enum rc viterbi(struct thread *t, struct imm_dp const *dp,
                       struct imm_task *task, struct imm_prod *prod,
                       double *loglik);
static int query_hmmer(struct thread *t, int prof_idx,
                       struct prot_profile const *prof);
static int write_product(struct thread *t, struct prot_profile const *prof);
static int write_hmmer(struct thread *t);

enum rc thread_run(struct thread *t)
{
    enum rc rc = RC_OK;

    struct profile *prof = 0;
    struct hypothesis *null = &t->null;
    struct hypothesis *alt = &t->alt;
    struct profile_reader *reader = t->reader;
    struct imm_seq const *seq = t->seq;

    rc = profile_reader_rewind(reader, t->idx);
    if (rc) goto cleanup;

    int prof_idx = t->prof_start;
    while ((rc = profile_reader_next(reader, t->idx, &prof)) == RC_OK)
    {
        struct imm_dp const *null_dp = profile_null_dp(prof);
        struct imm_dp const *alt_dp = profile_alt_dp(prof);
        struct prot_profile *pp = (struct prot_profile *)prof;
        unsigned size = imm_seq_size(seq);

        rc = setup_hypothesis(t, null, profile_null_dp(prof), seq);
        if (rc) goto cleanup;

        rc = setup_hypothesis(t, alt, profile_alt_dp(prof), seq);
        if (rc) goto cleanup;

        rc = prot_profile_setup(pp, size, t->cfg.multi_hits,
                                t->cfg.hmmer3_compat);
        if (rc) goto cleanup;

        rc = viterbi(t, null_dp, null->task, &null->prod, &t->prod.null_loglik);
        if (rc) goto cleanup;

        rc = viterbi(t, alt_dp, alt->task, &alt->prod, &t->prod.alt_loglik);
        if (rc) goto cleanup;

        progress_consume(&t->progress, 1);
        imm_float lrt = xmath_lrt(null->prod.loglik, alt->prod.loglik);

        if (!imm_lprob_is_finite(lrt) || lrt < t->cfg.lrt_threshold)
        {
            prof_idx += 1;
            continue;
        }

        strcpy(t->prod.profile_name, prof->accession);

        struct prot_profile const *pro = (struct prot_profile const *)prof;
        if ((rc = query_hmmer(t, prof_idx, pro))) break;
        if ((rc = write_product(t, pro))) break;
        if ((rc = write_hmmer(t))) break;

        prof_idx += 1;
    }
    if (rc == RC_END) rc = RC_OK;

cleanup:
    return rc;
}

struct progress const *thread_progress(struct thread const *t)
{
    return &t->progress;
}

char const *thread_errmsg(struct thread const *t) { return t->errmsg; }

void thread_cleanup(struct thread *t) { hmmer_result_del(t->hmmer_result); }

static enum rc reset_task(struct thread *t, struct imm_task **task,
                          struct imm_dp const *dp)
{
    if (*task && imm_task_reset(*task, dp))
        return efail("%s", errfmt(t->errmsg, "failed to reset task"));

    if (!*task && !(*task = imm_task_new(dp)))
        return efail("%s", errfmt(t->errmsg, "failed to create task"));

    return RC_OK;
}

static enum rc setup_task(struct thread *t, struct imm_task *task,
                          struct imm_seq const *seq)
{
    if (imm_task_setup(task, seq))
        return efail("%s", errfmt(t->errmsg, "failed to setup task"));
    return RC_OK;
}

typedef struct imm_dp const *(*profile_dp_func_t)(struct profile const *prof);

static enum rc setup_hypothesis(struct thread *t, struct hypothesis *hyp,
                                struct imm_dp const *dp,
                                struct imm_seq const *seq)
{
    enum rc rc = RC_OK;
    if ((rc = reset_task(t, &hyp->task, dp))) return rc;
    if ((rc = setup_task(t, hyp->task, seq))) return rc;
    imm_prod_reset(&hyp->prod);
    return rc;
}

static enum rc viterbi(struct thread *t, struct imm_dp const *dp,
                       struct imm_task *task, struct imm_prod *prod,
                       double *loglik)

{
    if (imm_dp_viterbi(dp, task, prod))
        return efail("%s", errfmt(t->errmsg, "failed to run viterbi"));
    *loglik = (double)prod->loglik;
    return RC_OK;
}

static int query_hmmer(struct thread *t, int prof_idx,
                       struct prot_profile const *prof)
{
    struct prot_match_iter iter = {0};
    struct prot_match const *match = NULL;

    prot_match_iter(&iter, prof, t->seq, &t->alt.prod.path);
    match = prot_match_iter_next(&iter);

    while ((match = prot_match_iter_next(&iter)))
    {
        char *y = t->amino_sequence;
        while ((match = prot_match_iter_next(&iter)))
        {
            if (!match->mute) *y++ = match->amino;
        }
        *y = '\0';
    }

    int rc = hmmer_client_put(0, prof_idx, t->amino_sequence, now() + 5000);
    if (rc) efail("hmmerc_put failure: %d", rc);

    rc = hmmer_client_pop(0, t->hmmer_result);
    if (rc) efail("hmmerc_pop failure: %d", rc);

    t->prod.evalue_log = hmmer_result_evalue_ln(t->hmmer_result);
    return 0;
}

static int write_product(struct thread *t, struct prot_profile const *prof)
{
    if (prod_write_begin(&t->prod, t->prodfile))
        return eio("failed to write prod");

    struct prot_match_iter iter = {0};
    prot_match_iter(&iter, prof, t->seq, &t->alt.prod.path);
    struct prot_match const *match = prot_match_iter_next(&iter);
    if (prot_match_write(match, t->prodfile)) return eio("write prod");
    while ((match = prot_match_iter_next(&iter)))
    {
        if (prod_write_sep(t->prodfile)) return eio("failed to write prod");
        if (prot_match_write(match, t->prodfile)) return eio("write prod");
    }
    if (prod_write_end(t->prodfile)) return eio("failed to write prod");
    return 0;
}

static int write_hmmer(struct thread *t)
{
    char filename[FILENAME_SIZE] = {0};
    struct prod const *prod = &t->prod;

    if (fs_mkdir("prod", true)) return eio("could not create dir");
    if (fs_mkdir("prod/hmmer", true)) return eio("could not create dir");
    sprintf(filename, "prod/hmmer/%ld", prod->seq_id);
    if (fs_mkdir(filename, true)) return eio("could not create dir");

    prod_hmmer_filename(filename, prod->seq_id, prod->profile_name);

    FILE *fp = fopen(filename, "wb");
    if (!fp) return eio("could not open file %s", filename);

    if (hmmer_result_write(t->hmmer_result, fp))
    {
        fclose(fp);
        return eio("could not write hmmer results");
    }

    return fclose(fp) ? eio("could not close file") : 0;

    return 0;
}

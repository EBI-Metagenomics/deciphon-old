#include "scan/thread.h"
#include "core/errmsg.h"
#include "core/logy.h"
#include "core/progress.h"
#include "core/xmath.h"
#include "db/profile_reader.h"
#include "scan/cfg.h"

void thread_init(struct thread *t, FILE *prodfile, int idx,
                 struct profile_reader *reader, struct scan_cfg cfg,
                 prod_fwrite_match_fn_t *write_match_cb)
{
    t->prodfile = prodfile;
    t->idx = idx;
    t->seq = NULL;
    t->reader = reader;
    t->cfg = cfg;
    t->write_match_cb = write_match_cb;
    progress_init(&t->progress, 0);
    t->errnum = RC_OK;
    t->errmsg[0] = '\0';
}

void thread_setup_job(struct thread *t, enum imm_abc_typeid abc_typeid,
                      enum profile_typeid profile_typeid, int64_t scan_id,
                      long ntasks)
{
    char const *abc = imm_abc_typeid_name(abc_typeid);
    char const *prof = profile_typeid_name(profile_typeid);
    prod_setup_job(&t->prod, abc, prof, scan_id);
    progress_init(&t->progress, ntasks);
}

void thread_setup_seq(struct thread *t, struct imm_seq const *seq,
                      int64_t seq_id)
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
static enum rc write_product(struct thread const *t,
                             struct imm_path const *path);

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

    while ((rc = profile_reader_next(reader, t->idx, &prof)) == RC_OK)
    {
        struct imm_dp const *null_dp = profile_null_dp(prof);
        struct imm_dp const *alt_dp = profile_alt_dp(prof);
        struct protein_profile *pp = (struct protein_profile *)prof;
        unsigned size = imm_seq_size(seq);

        rc = setup_hypothesis(t, null, profile_null_dp(prof), seq);
        if (rc) goto cleanup;

        rc = setup_hypothesis(t, alt, profile_alt_dp(prof), seq);
        if (rc) goto cleanup;

        rc = protein_profile_setup(pp, size, t->cfg.multi_hits,
                                   t->cfg.hmmer3_compat);
        if (rc) goto cleanup;

        rc = viterbi(t, null_dp, null->task, &null->prod, &t->prod.null_loglik);
        if (rc) goto cleanup;

        rc = viterbi(t, alt_dp, alt->task, &alt->prod, &t->prod.alt_loglik);
        if (rc) goto cleanup;

        progress_consume(&t->progress, 1);
        imm_float lrt = xmath_lrt(null->prod.loglik, alt->prod.loglik);

        if (!imm_lprob_is_finite(lrt) || lrt < t->cfg.lrt_threshold) continue;

        strcpy(t->prod.profile_name, prof->accession);

        match_setup((struct match *)&t->match, prof);
        rc = write_product(t, &alt->prod.path);
        if (rc) goto cleanup;
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

static enum rc write_product(struct thread const *t,
                             struct imm_path const *path)
{
    struct match *match = (struct match *)&t->match;
    prod_fwrite_match_fn_t *fn = t->write_match_cb;
    return prod_write(&t->prod, t->seq, path, fn, match, t->prodfile);
}

#include "thread.h"
#include "deciphon/db/profile_reader.h"
#include "deciphon/logger.h"
#include "deciphon/server/rest.h"
#include "deciphon/version.h"
#include "deciphon/xmath.h"

void thread_init(struct thread *t, unsigned id, struct profile_reader *reader,
                 bool multi_hits, bool hmmer3_compat, imm_float lrt_threshold,
                 prod_fwrite_match_func_t write_match_func)
{
    t->id = id;
    t->seq = 0;
    t->reader = reader;

    t->multi_hits = multi_hits;
    t->hmmer3_compat = hmmer3_compat;
    t->lrt_threshold = lrt_threshold;

    t->write_match_func = write_match_func;
}

void thread_setup_job(struct thread *t, enum imm_abc_typeid abc_typeid,
                      enum profile_typeid profile_typeid, unsigned job_id)
{
    char const *abc = imm_abc_typeid_name(abc_typeid);
    char const *prof = profile_typeid_name(profile_typeid);
    prod_setup_job(&t->prod, abc, prof, job_id);
}

void thread_setup_seq(struct thread *t, struct imm_seq *seq, unsigned seq_id)
{
    t->seq = seq;
    prod_setup_seq(&t->prod, seq_id);
}

static enum rc reset_task(struct imm_task **task, struct imm_dp const *dp)
{
    if (*task && imm_task_reset(*task, dp))
        return efail("failed to reset task");

    if (!*task && !(*task = imm_task_new(dp)))
        return efail("failed to create task");

    return RC_OK;
}

static enum rc setup_task(struct imm_task *task, struct imm_seq const *seq)
{
    if (imm_task_setup(task, seq)) return efail("failed to setup task");
    return RC_OK;
}

typedef struct imm_dp const *(*profile_dp_func_t)(struct profile const *prof);

static enum rc setup_hypothesis(struct hypothesis *hyp, struct imm_dp const *dp,
                                struct imm_seq const *seq)
{
    enum rc rc = RC_OK;
    if ((rc = reset_task(&hyp->task, dp))) return rc;
    if ((rc = setup_task(hyp->task, seq))) return rc;
    imm_prod_reset(&hyp->prod);
    return rc;
}

static enum rc viterbi(struct imm_dp const *dp, struct imm_task *task,
                       struct imm_prod *prod, double *loglik)

{
    if (imm_dp_viterbi(dp, task, prod)) return efail("failed to run viterbi");
    *loglik = (double)prod->loglik;
    return RC_OK;
}

static enum rc write_product(struct thread *t, struct imm_path const *path)
{
    struct match *match = (struct match *)&t->match;
    prod_fwrite_match_func_t func = t->write_match_func;
    return prod_fwrite(&t->prod, t->seq, path, t->id, func, match);
}

static inline enum rc fail_job(enum rc rc, struct thread *t, char const *msg)
{
    struct rest_error rerr = {0};
    rest_set_job_state(t->prod.job_id, SCHED_JOB_FAIL, msg, &rerr);
    return rc;
}

enum rc thread_run(struct thread *t)
{
    enum rc rc = RC_OK;

    struct profile *prof = 0;
    struct hypothesis *null = &t->null;
    struct hypothesis *alt = &t->alt;
    struct profile_reader *reader = t->reader;
    struct imm_seq const *seq = t->seq;

    while ((rc = profile_reader_next(reader, t->id, &prof)) == RC_OK)
    {
        // if (atomic_load(&end_work)) break;
        //
        struct imm_dp const *null_dp = profile_null_dp(prof);
        struct imm_dp const *alt_dp = profile_alt_dp(prof);
        struct protein_profile *pp = (struct protein_profile *)prof;
        unsigned size = imm_seq_size(seq);

        rc = setup_hypothesis(null, profile_null_dp(prof), seq);
        if (rc) return fail_job(rc, t, "failed to setup null hypothesis");

        rc = setup_hypothesis(alt, profile_alt_dp(prof), seq);
        if (rc) return fail_job(rc, t, "failed to setup alt hypothesis");

        rc = protein_profile_setup(pp, size, t->multi_hits, t->hmmer3_compat);
        if (rc) return fail_job(rc, t, "failed to setup profile");

        rc = viterbi(null_dp, null->task, &null->prod, &t->prod.null_loglik);
        if (rc) return fail_job(rc, t, "failed to run viterbi");

        rc = viterbi(alt_dp, alt->task, &alt->prod, &t->prod.alt_loglik);
        if (rc) return fail_job(rc, t, "failed to run viterbi");

        imm_float lrt = xmath_lrt(null->prod.loglik, alt->prod.loglik);

        if (!imm_lprob_is_finite(lrt) || lrt < t->lrt_threshold) continue;

        strcpy(t->prod.profile_name, prof->accession);

        match_setup((struct match *)&t->match, prof);
        rc = write_product(t, &alt->prod.path);
        if (rc) return fail_job(rc, t, "failed to write product");
    }
    return RC_OK;
}

enum rc thread_finishup(struct thread *t)
{
    enum rc rc = prod_fclose();
    if (rc) return fail_job(rc, t, "failed to end product submission");

    struct rest_error rerr = {0};
    rc = rest_set_job_state(t->prod.job_id, SCHED_JOB_DONE, "", &rerr);
    if (rc) return rc;

    return rerr.rc ? erest(rerr.msg) : RC_OK;
}

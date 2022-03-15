#include "thread.h"
#include "deciphon/db/profile_reader.h"
#include "deciphon/logger.h"
#include "deciphon/version.h"
#include "deciphon/xmath.h"

void thread_init(struct thread *t, unsigned id, struct profile_reader *reader,
                 bool multi_hits, bool hmmer3_compat, imm_float lrt_threshold)
{
    t->id = id;
    t->iseq = 0;
    t->reader = reader;

    t->multi_hits = multi_hits;
    t->hmmer3_compat = hmmer3_compat;
    t->lrt_threshold = lrt_threshold;
}

void thread_setup_job(struct thread *t, enum imm_abc_typeid abc_typeid,
                      enum profile_typeid profile_typeid, unsigned job_id)
{
    char const *abc = imm_abc_typeid_name(abc_typeid);
    char const *prof = profile_typeid_name(profile_typeid);
    prod_setup_job(&t->prod, abc, prof, job_id);
}

void thread_setup_seq(struct thread *t, unsigned seq_id)
{
    prod_setup_seq(&t->prod, seq_id);
}

static enum rc reset_task(struct imm_task **task, struct imm_dp const *dp)
{
    if (*task && imm_task_reset(*task, dp))
        return error(RC_EFAIL, "failed to reset task");

    if (!*task && !(*task = imm_task_new(dp)))
        return error(RC_EFAIL, "failed to create task");

    return RC_OK;
}

static enum rc setup_task(struct imm_task *task, struct imm_seq const *seq)
{
    if (imm_task_setup(task, seq))
        return error(RC_EFAIL, "failed to setup task");
    return RC_OK;
}

typedef struct imm_dp const *(*profile_dp_func_t)(struct profile const *prof);

static enum rc setup_hypothesis(struct hypothesis *hyp,
                                profile_dp_func_t dp_func, struct profile *prof,
                                struct imm_seq *iseq)
{
    enum rc rc = RC_OK;
    if ((rc = reset_task(&hyp->task, dp_func(prof)))) return rc;
    if ((rc = setup_task(hyp->task, iseq))) return rc;
    imm_prod_reset(&hyp->prod);
    return rc;
}

static enum rc viterbi(struct profile *prof, struct imm_task *task,
                       struct imm_prod *prod, double *loglik)

{
    if (imm_dp_viterbi(profile_null_dp(prof), task, prod))
        return efail("failed to run viterbi");

    *loglik = (double)prod->loglik;
    return RC_OK;
}

static enum rc write_product(struct thread *wt, struct profile *prof,
                             struct imm_path const *path)
{
    struct match *match = (struct match *)&wt->match;
    match->profile = prof;
    enum rc rc = prod_fwrite(&wt->prod, wt->iseq, path, wt->id,
                             wt->write_match_func, (struct match *)&wt->match);
    return rc;
}

enum rc thread_run(struct thread *t, struct imm_seq *iseq)
{
    enum rc rc = RC_OK;

    struct profile *prof = 0;
    struct hypothesis *null = &t->null;
    struct hypothesis *alt = &t->alt;
    struct profile_reader *reader = t->reader;

    while ((rc = profile_reader_next(reader, t->id, &prof)) == RC_OK)
    {
        // if (atomic_load(&end_work)) break;

        if ((rc = setup_hypothesis(null, profile_null_dp, prof, iseq)))
            return rc;
        if ((rc = setup_hypothesis(alt, profile_alt_dp, prof, iseq))) return rc;

        struct protein_profile *pp = (struct protein_profile *)prof;
        unsigned sz = imm_seq_size(iseq);
        rc = protein_profile_setup(pp, sz, t->multi_hits, t->hmmer3_compat);

        rc = viterbi(prof, null->task, &null->prod, &t->prod.null_loglik);
        rc = viterbi(prof, alt->task, &alt->prod, &t->prod.alt_loglik);

        imm_float lrt = xmath_lrt(null->prod.loglik, alt->prod.loglik);

        if (!imm_lprob_is_finite(lrt) || lrt < t->lrt_threshold) continue;

        // strcpy(wt->prod.profile_name, mt.acc);
        // strcpy(wt->prod.abc_name, wt->abc_name);

        rc = write_product(t, prof, &alt->prod.path);
    }
    return RC_OK;
}

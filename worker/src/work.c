#include "work.h"
#include "common/logger.h"
#include "common/xmath.h"
#include "imm/imm.h"
#include "prod.h"
#include "profile_reader.h"
#include "protein_match.h"
#include "rest.h"
#include "standard_match.h"
#include "version.h"
#include "xomp.h"
#include <stdatomic.h>
#include <string.h>

static atomic_flag lock_work = ATOMIC_FLAG_INIT;
static atomic_bool end_work = false;

static void lock(void)
{
    while (atomic_flag_test_and_set(&lock_work))
        /* spin until the lock is acquired */;
}

static void unlock(void) { atomic_flag_clear(&lock_work); }

static void set_job_fail(struct sched_job *job, char const *msg)
{
    lock();
    end_work = true;
    if (rest_set_job_state(job, SCHED_JOB_FAIL, msg))
        error(RC_EFAIL, "failed to set job_fail");
    unlock();
}

static enum rc reset_task(struct imm_task **task, struct imm_dp const *dp)
{
    if (*task && imm_task_reset(*task, dp))
        return error(RC_EFAIL, "failed to reset task");

    if (!*task && !(*task = imm_task_new(dp)))
        return error(RC_EFAIL, "failed to create task");

    return RC_DONE;
}

static enum rc setup_task(struct imm_task *task, struct imm_seq const *seq)
{
    if (imm_task_setup(task, seq))
        return error(RC_EFAIL, "failed to setup task");
    return RC_DONE;
}

static enum rc run_on_partition(struct work *work, struct work_priv *priv,
                                unsigned i)
{
    struct profile *prof = 0;
    struct hypothesis *null = &priv->null;
    struct hypothesis *alt = &priv->alt;
    enum rc rc = RC_DONE;
    while ((rc = profile_reader_next(&work->reader, i, &prof)) == RC_NEXT)
    {
        printf(".");
        if (atomic_load(&end_work)) break;

        if ((rc = reset_task(&null->task, profile_null_dp(prof)))) return rc;

        if ((rc = reset_task(&alt->task, profile_alt_dp(prof)))) return rc;

        if ((rc = setup_task(null->task, &work->iseq))) return rc;
        if ((rc = setup_task(alt->task, &work->iseq))) return rc;

        imm_prod_reset(&null->prod);
        imm_prod_reset(&alt->prod);

        protein_profile_setup((struct protein_profile *)prof,
                              imm_seq_size(&work->iseq), work->job.multi_hits,
                              work->job.hmmer3_compat);

        if (imm_dp_viterbi(profile_null_dp(prof), null->task, &null->prod))
            return error(RC_EFAIL, "failed to run viterbi");

        if (imm_dp_viterbi(profile_alt_dp(prof), alt->task, &alt->prod))
            return error(RC_EFAIL, "failed to run viterbi");

        imm_float lrt = xmath_lrt(null->prod.loglik, alt->prod.loglik);

        if (!imm_lprob_is_finite(lrt) || lrt < work->lrt_threshold) continue;

        struct metadata const mt =
            db_metadata((struct db const *)&work->db.reader.db,
                        (unsigned)prof->idx_within_db);
        strcpy(priv->prod.profile_name, mt.acc);
        strcpy(priv->prod.abc_name, work->abc_name);

        priv->prod.alt_loglik = alt->prod.loglik;
        priv->prod.null_loglik = null->prod.loglik;

        struct imm_path const *path = &alt->prod.path;
        struct match *match = (struct match *)&priv->match;
        match->profile = prof;
        if ((rc = prod_fwrite(&priv->prod, &work->iseq, path, i,
                              work->write_match_cb,
                              (struct match *)&work->priv->match)))
            return rc;
    }
    printf("\n");
    return RC_DONE;
}

enum rc work_next(struct work *work)
{
    enum rc rc = rest_next_pend_job(&work->job);
    if (rc) return rc;
    rc = rest_set_job_state(&work->job, SCHED_JOB_RUN, "");
    if (rc) return rc;

    struct sched_db db = {0};
    db.id = work->job.db_id;
    rc = rest_get_db(&db);
    if (rc) return rc;

    if (!(work->db.fp = fopen(db.filename, "rb"))) return eio("open db");

    rc = db_reader_open(&work->db.reader, work->db.fp);
    if (rc)
    {
        fclose(work->db.fp);
        return rc;
    }

    if (db_reader_db(&work->db.reader)->vtable.typeid == DB_PROTEIN)
    {
        work->write_match_cb = protein_match_write_cb;
        work->profile_typeid = PROFILE_PROTEIN;
    }
    else if (db_reader_db(&work->db.reader)->vtable.typeid == DB_STANDARD)
    {
        work->write_match_cb = standard_match_write_cb;
        work->profile_typeid = PROFILE_STANDARD;
    }
    else
        assert(false);

    return RC_NEXT;
}

enum rc work_run(struct work *work, unsigned num_threads)
{
    enum rc rc = profile_reader_setup(
        &work->reader, db_reader_db(&work->db.reader), num_threads);
    if (rc)
    {
        db_reader_close(&work->db.reader);
        fclose(work->db.fp);
        return rc;
    }

    work->abc = db_abc(db_reader_db(&work->db.reader));
    if (work->abc->vtable.typeid == IMM_DNA)
        strcpy(work->abc_name, "dna");
    else if (work->abc->vtable.typeid == IMM_RNA)
        strcpy(work->abc_name, "rna");
    else
    {
        set_job_fail(&work->job, "unknown alphabet");
        return error(RC_EINVAL, "unknown alphabet");
    }

    work->seq.id = 0;
    work->seq.job_id = work->job.id;
    unsigned npartitions = profile_reader_npartitions(&work->reader);
    if (prod_fopen(npartitions))
    {
        set_job_fail(&work->job, "failed to begin product submission");
        return RC_DONE;
    }
    while (!(rc = rest_next_seq(&work->seq)))
    {
        if (imm_abc_union_size(work->abc, imm_str(work->seq.data)) > 0)
        {
            set_job_fail(&work->job, "sequence has out-of-alphabet characters");
            return RC_DONE;
        }

        work->iseq = imm_seq(imm_str(work->seq.data), work->abc);
        // _Pragma("omp parallel default(none) shared(work, npartitions)
        // if(npartitions > 1)")
        // {
        //     _Pragma("omp single")
        //     {
        if (profile_reader_rewind(&work->reader))
        {
            set_job_fail(&work->job, "failed to rewind profile db");
        }
        else
        {
#pragma omp parallel for
            for (unsigned i = 0; i < npartitions; ++i)
            {
                work->priv[i].prod.job_id = work->job.id;
                work->priv[i].prod.seq_id = work->seq.id;
                strcpy(work->priv[i].prod.profile_typeid,
                       profile_typeid_name(work->profile_typeid));
                strcpy(work->priv[i].prod.version, VERSION);

                // _Pragma("omp task firstprivate(i)")
                // {
                printf("Partition: %d\n", i);
                fflush(stdout);
                run_on_partition(work, work->priv + i, i);
                printf("Depois Partition: %d\n", i);
                fflush(stdout);
                // }
            }
        }
        //     }
        // }
    }
    if (prod_fclose())
    {
        set_job_fail(&work->job, "failed to end product submission");
        return RC_DONE;
    }

    if (rest_set_job_state(&work->job, SCHED_JOB_DONE, ""))
        error(RC_EFAIL, "failed to set job_done");
    return RC_DONE;
}

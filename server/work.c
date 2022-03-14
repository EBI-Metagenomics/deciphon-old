#include "work.h"
#include "deciphon/db/profile_reader.h"
#include "deciphon/logger.h"
#include "deciphon/server/rest.h"
#include "deciphon/version.h"
#include "deciphon/xfile.h"
#include "imm/imm.h"
#include "prod.h"
#include "protein_match.h"
#include "xomp.h"
#include <stdatomic.h>
#include <string.h>

// static atomic_flag lock_work = ATOMIC_FLAG_INIT;
// static atomic_bool end_work = false;
//
// static void lock(void)
// {
//     while (atomic_flag_test_and_set(&lock_work))
//         /* spin until the lock is acquired */;
// }
//
// static void unlock(void) { atomic_flag_clear(&lock_work); }

#if 0
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

    return RC_OK;
}

static enum rc setup_task(struct imm_task *task, struct imm_seq const *seq)
{
    if (imm_task_setup(task, seq))
        return error(RC_EFAIL, "failed to setup task");
    return RC_OK;
}

static enum rc run_on_partition(struct work *work, struct work_priv *priv,
                                unsigned i)
{
    struct profile *prof = 0;
    struct hypothesis *null = &priv->null;
    struct hypothesis *alt = &priv->alt;
    enum rc rc = RC_OK;
    while ((rc = profile_reader_next(&work->reader, i, &prof)) == RC_OK)
    {
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

        // strcpy(priv->prod.profile_name, mt.acc);
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
    return RC_OK;
}
#endif

enum rc work_next(struct work *work)
{
    struct rest_error error = {0};
    enum rc rc = rest_next_pend_job(&work->job, &error);
    if (rc) return rc;
    return rest_set_job_state(&work->job, SCHED_JOB_RUN, "", &error);
}

static enum rc ensure_database_integrity(char const *filename, int64_t xxh3_64)
{
    union
    {
        uint64_t u64;
        int64_t i64;
    } hash = {.u64 = 0};

    FILE *fp = fopen(filename, "rb");
    enum rc rc = xfile_hash(fp, &hash.u64);
    if (rc)
    {
        fclose(fp);
        return rc;
    }
    fclose(fp);
    return xxh3_64 == hash.i64 ? RC_OK : einval("wrong hash");
}

static enum rc download_database(struct sched_db *db)
{
    FILE *fp = fopen(db->filename, "wb");
    enum rc rc = rest_download_db(db, fp);
    if (rc)
    {
        fclose(fp);
        return rc;
    }
    return fclose(fp) ? eio("failed to close database") : RC_OK;
}

static enum rc ensure_database(struct sched_db *db)
{
    enum rc rc = RC_OK;
    if (xfile_exists(db->filename))
    {
        rc = ensure_database_integrity(db->filename, db->xxh3_64);
        if (!rc) return rc;
    }

    rc = download_database(db);
    if (rc) return rc;

    return ensure_database_integrity(db->filename, db->xxh3_64);
}

static inline void fail_job(struct work *work, char const *msg)
{
    rest_set_job_state(&work->job, SCHED_JOB_FAIL, msg, 0);
}

enum rc work_prepare(struct work *work, unsigned num_threads)
{
    FILE *fp = 0;

    struct sched_db db = {0};
    db.id = work->job.db_id;

    struct rest_error error = {0};
    enum rc rc = rest_get_db(&db, &error);
    if (rc)
    {
        fail_job(work, error.msg);
        goto cleanup;
    }
    rc = ensure_database(&db);
    if (rc)
    {
        fail_job(work, "failed to ensure database");
        goto cleanup;
    }

    fp = fopen(db.filename, "rb");
    rc = protein_db_reader_open(&work->db_reader, fp);
    if (rc)
    {
        fail_job(work, "failed to setup database reader");
        goto cleanup;
    }

    struct profile_reader *profile_reader = &work->profile_reader;
    struct db_reader *db_reader = (struct db_reader *)&work->db_reader;
    rc = profile_reader_setup(profile_reader, db_reader, num_threads);
    if (rc) goto cleanup;

    return RC_OK;

cleanup:
    if (fp) fclose(fp);
    return rc;
}

enum rc work_run(struct work *work)
{
    enum rc rc = RC_OK;
    struct imm_abc const *abc = imm_super(&work->db_reader.nuclt);

    struct profile *prof = 0;
    struct imm_prod prod = imm_prod();
    while ((rc = profile_reader_next(&work->profile_reader, 0, &prof)) !=
           RC_END)
    {
        // EQ(profile_typeid(prof), PROFILE_PROTEIN);
        struct imm_task *task = imm_task_new(profile_alt_dp(prof));
        struct imm_seq seq = imm_seq(imm_str(imm_example2_seq), abc);
        imm_task_setup(task, &seq);
        imm_dp_viterbi(profile_alt_dp(prof), task, &prod);
        // CLOSE(prod.loglik, logliks[nprofs]);
        imm_del(task);
    }

    imm_del(&prod);
    // profile_reader_del(&reader);
    // db_reader_close((struct db_reader *)&db);
    // fclose(fp);

#if 0
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
        return einval("unknown alphabet");
    }

    work->seq.id = 0;
    work->seq.job_id = work->job.id;
    unsigned npartitions = profile_reader_npartitions(&work->reader);
    if (prod_fopen(npartitions))
    {
        set_job_fail(&work->job, "failed to begin product submission");
        return RC_OK;
    }
    while (!(rc = rest_next_seq(&work->seq)))
    {
        if (imm_abc_union_size(work->abc, imm_str(work->seq.data)) > 0)
        {
            set_job_fail(&work->job, "sequence has out-of-alphabet characters");
            return RC_OK;
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
                strcpy(work->priv[i].prod.version, DECIPHON_VERSION);

                // _Pragma("omp task firstprivate(i)")
                // {
                run_on_partition(work, work->priv + i, i);
                // }
            }
        }
        //     }
        // }
    }
    if (prod_fclose())
    {
        set_job_fail(&work->job, "failed to end product submission");
        return RC_OK;
    }

    if (rest_set_job_state(&work->job, SCHED_JOB_DONE, ""))
        error(RC_EFAIL, "failed to set job_done");
#endif
    return RC_OK;
}

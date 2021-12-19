#include "work.h"
#include "dcp_sched/sched.h"
#include "imm/imm.h"
#include "logger.h"
#include "prod.h"
#include "profile_reader.h"
#include "protein_match.h"
#include "standard_match.h"
#include "version.h"
#include "xmath.h"
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

static void set_job_fail(int64_t job_id, char const *msg)
{
    lock();
    end_work = true;
    if (sched_set_job_fail(job_id, msg))
        error(RC_FAIL, "failed to set job_fail");
    unlock();
}

static enum rc reset_task(struct imm_task **task, struct imm_dp const *dp)
{
    if (*task && imm_task_reset(*task, dp))
        return error(RC_FAIL, "failed to reset task");

    if (!*task && !(*task = imm_task_new(dp)))
        return error(RC_FAIL, "failed to create task");

    return RC_DONE;
}

static enum rc setup_task(struct imm_task *task, struct imm_seq const *seq)
{
    if (imm_task_setup(task, seq))
        return error(RC_FAIL, "failed to setup task");
    return RC_DONE;
}

static enum rc run_on_partition(struct work *work, unsigned i)
{
    struct profile *prof = 0;
    struct hypothesis *null = &work->null;
    struct hypothesis *alt = &work->alt;
    enum rc rc = RC_DONE;
    while ((rc = profile_reader_next(&work->reader, i, &prof)) == RC_NEXT)
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
            return error(RC_FAIL, "failed to run viterbi");

        if (imm_dp_viterbi(profile_alt_dp(prof), alt->task, &alt->prod))
            return error(RC_FAIL, "failed to run viterbi");

        imm_float lrt = xmath_lrt(null->prod.loglik, alt->prod.loglik);

        if (!imm_lprob_is_finite(lrt) || lrt < work->lrt_threshold) continue;

        struct metadata const mt =
            db_metadata((struct db const *)&work->db.reader.db,
                        (unsigned)prof->idx_within_db);
        strcpy(work->prod->profile_name, mt.acc);
        strcpy(work->prod->abc_name, work->abc_name);

        work->prod->alt_loglik = alt->prod.loglik;
        work->prod->null_loglik = null->prod.loglik;

        struct imm_path const *path = &alt->prod.path;
        struct match *match = (struct match *)&work->match;
        match->profile = prof;
        if ((rc = prod_write(work->prod + i, &work->iseq, path, i,
                             work->write_match_cb,
                             (struct match *)&work->match)))
            return rc;
    }
    return RC_DONE;
}

enum rc work_next(struct work *work)
{
    int code = sched_next_pending_job(&work->job);
    if (code == SCHED_NOTFOUND) return RC_DONE;
    if (code != SCHED_DONE)
        return error(RC_FAIL, "failed to get next pending job");

    char filepath[DCP_PATH_SIZE] = {0};
    code = sched_cpy_db_filepath(DCP_PATH_SIZE, filepath, work->job.db_id);
    if (code == SCHED_NOTFOUND) return error(RC_NOTFOUND, "db not found");
    if (code != SCHED_DONE) return error(RC_FAIL, "failed to get db filepath");

    if (!(work->db.fp = fopen(filepath, "rb")))
        return error(RC_IOERROR, "failed to open db");

    enum rc rc = db_reader_open(&work->db.reader, work->db.fp);
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
        set_job_fail(work->job.id, "unknown alphabet");
        return error(RC_ILLEGALARG, "unknown alphabet");
    }

    sched_seq_init(&work->seq, work->job.id, "", "");
    sched_seq_init(&work->seq, work->job.id, "", "");
    unsigned npartitions = profile_reader_npartitions(&work->reader);
    int code = SCHED_DONE;
    if (sched_begin_prod_submission(npartitions))
    {
        set_job_fail(work->job.id, "failed to begin product submission");
        return RC_DONE;
    }
    while ((code = sched_seq_next(&work->seq)) == SCHED_NEXT)
    {
        if (imm_abc_union_size(work->abc, imm_str(work->seq.data)) > 0)
        {
            set_job_fail(work->job.id,
                         "sequence has out-of-alphabet characters");
            return RC_DONE;
        }

        work->iseq = imm_seq(imm_str(work->seq.data), work->abc);
        /* _Pragma("omp parallel shared(work) if(0)") */
        /* _Pragma("omp parallel shared(work) if(npartitions > 1)") */
        {
            /* _Pragma("omp single") */
            {
                if (profile_reader_rewind(&work->reader))
                {
                    set_job_fail(work->job.id, "failed to rewind profile db");
                    return RC_DONE;
                }
                for (unsigned i = 0; i < npartitions; ++i)
                {
                    work->prod[i].job_id = work->job.id;
                    work->prod[i].seq_id = work->seq.id;
                    strcpy(work->prod[i].profile_typeid,
                           profile_typeid_name(work->profile_typeid));
                    strcpy(work->prod[i].version, VERSION);
                    /* _Pragma("omp task firstprivate(rc, i)") */
                    {
                        rc = run_on_partition(work, i);
                    }
                }
            }
        }
    }
    if (sched_end_prod_submission())
    {
        set_job_fail(work->job.id, "failed to end product submission");
        return RC_DONE;
    }

    if (sched_set_job_done(work->job.id))
        error(RC_FAIL, "failed to set job_done");
    return RC_DONE;
}

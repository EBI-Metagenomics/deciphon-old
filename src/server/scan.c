#include "scan.h"
#include "deciphon/core/logging.h"
#include "deciphon/core/progress.h"
#include "deciphon/core/rc.h"
#include "deciphon/core/xfile.h"
#include "deciphon/core/xmath.h"
#include "deciphon/db/db.h"
#include "deciphon/db/profile_reader.h"
#include "deciphon/sched/api.h"
#include "deciphon/sched/sched.h"
#include "file.h"
#include "imm/imm.h"
#include "job.h"
#include "prod.h"
#include "protein_match.h"
#include "scan_thread.h"
#include "xomp.h"
#include <stdatomic.h>
#include <string.h>

struct scan
{
    struct sched_scan sched;
    struct sched_seq seq;

    unsigned num_threads;
    struct scan_thread thread[NUM_THREADS];

    char db_filename[SCHED_FILENAME_SIZE];
    struct protein_db_reader db_reader;
    struct profile_reader profile_reader;

    struct imm_str str;
    struct imm_seq iseq;
    struct imm_abc const *abc;

    prod_fwrite_match_func_t write_match_func;
    double lrt_threshold;
};

static struct scan scan = {0};
static struct api_error api_rc = {0};
static struct sched_db db = {0};

static enum rc prepare_readers(void)
{
    FILE *fp = fopen(db.filename, "rb");
    if (!fp)
    {
        job_set_fail(scan.sched.job_id, "failed to open database");
        return eio("failed to open database");
    }

    enum rc rc = protein_db_reader_open(&scan.db_reader, fp);
    if (rc)
    {
        job_set_fail(scan.sched.job_id, "failed to setup database reader");
        goto cleanup;
    }

    struct profile_reader *profile_reader = &scan.profile_reader;
    struct db_reader *db_reader = (struct db_reader *)&scan.db_reader;

    rc = profile_reader_setup(profile_reader, db_reader, scan.num_threads);
    if (rc) goto cleanup;

    scan.abc = (struct imm_abc const *)&scan.db_reader.nuclt;

    return RC_OK;

cleanup:
    if (fp) fclose(fp);
    return rc;
}

static enum rc fetch_db(char const *filename, int64_t xxh3)
{
    FILE *fp = fopen(filename, "wb");
    if (!fp) return eio("fopen");
    // struct api_error api_rc = {0};
    enum rc rc = api_download_db(scan.sched.db_id, fp);
    if (rc)
    {
        job_set_fail(scan.sched.job_id, "failed to download database");
        fclose(fp);
        return rc;
    }
    if (fclose(fp))
    {
        job_set_fail(scan.sched.job_id, "failed to close database file");
        return eio("fclose");
    }
    return RC_OK;
}

static void send_progress(unsigned long units, void *data)
{
    // struct api_error api_rc = {0};
    int64_t *job_id = data;
    api_increment_job_progress(*job_id, (int)units);
}

static enum rc compute_number_of_tasks(unsigned *total)
{
    enum rc rc = RC_OK;
    unsigned nseqs = 0;
    if ((rc = api_scan_num_seqs(scan.sched.id, &nseqs))) return rc;

    *total = nseqs * profile_reader_nprofiles(&scan.profile_reader);
    return rc;
}

static enum rc scan_init(unsigned num_threads, double lrt_threshold)
{
    scan.num_threads = num_threads;
    scan.lrt_threshold = lrt_threshold;

    enum rc rc = RC_OK;

    if ((rc = prod_fopen(num_threads)))
    {
        job_set_fail(scan.sched.job_id, "failed to open product files");
        goto cleanup;
    }

    if ((rc = api_get_db(scan.sched.db_id, &db)))
    {
        job_set_fail(scan.sched.job_id, "failed to get database");
        return rc;
    }
    // if (api_rc.rc)
    // {
    //     job_set_fail(scan.sched.job_id, api_rc.msg);
    //     return rc;
    // }

    info("Ensuring database exists locally");
    if ((rc = file_ensure_local(db.filename, db.xxh3, fetch_db)))
    {
        job_set_fail(scan.sched.job_id, "failed to have database on disk");
        return rc;
    }

    info("Preparing readers");
    if ((rc = prepare_readers()))
    {
        prod_fcleanup();
        goto cleanup;
    }
    scan.write_match_func = protein_match_write_func;

    unsigned npartitions = profile_reader_npartitions(&scan.profile_reader);
    for (unsigned i = 0; i < npartitions; ++i)
    {
        struct scan_thread *t = scan.thread + i;
        struct profile_reader *reader = &scan.profile_reader;
        bool multi_hits = scan.sched.multi_hits;
        bool hmmer3_compat = scan.sched.hmmer3_compat;
        double lrt = scan.lrt_threshold;
        prod_fwrite_match_func_t func = scan.write_match_func;

        thread_init(t, i, reader, multi_hits, hmmer3_compat, lrt, func);

        enum imm_abc_typeid abc_typeid = scan.abc->vtable.typeid;
        enum profile_typeid profile_typeid = reader->profile_typeid;
        thread_setup_job(t, abc_typeid, profile_typeid, scan.sched.id);
    }

    unsigned nseqs = 0;
    if ((rc = api_scan_num_seqs(scan.sched.id, &nseqs))) return rc;

    unsigned total = 0;
    if ((rc = compute_number_of_tasks(&total)))
    {
        job_set_fail(scan.sched.job_id, "failed to compute number of tasks");
        goto cleanup;
    }
    info("%d tasks to run", total);

    progress_setup(
        npartitions, total, 100,
        (struct progress_callback){send_progress, &scan.sched.job_id});

cleanup:
    return rc;
}

static enum rc work_finishup(int64_t job_id)
{
    progress_finishup();
    enum rc rc = prod_fclose();
    if (rc)
    {
        job_set_fail(job_id, "failed to finish up work");
        goto cleanup;
    }

    char const *filepath = prod_final_path();

    if ((rc = api_upload_prods_file(filepath)))
    {
        job_set_fail(job_id, "failed to submit prods_file");
        goto cleanup;
    }

    rc = api_set_job_state(job_id, SCHED_DONE, "");
    if (rc) return rc;

    // return api_rc.rc ? eapi(api_rc) : RC_OK;
    return RC_OK;

cleanup:
    return rc;
}

enum rc scan_run(int64_t job_id, unsigned num_threads)
{
    enum rc rc = RC_OK;

    if ((rc = api_get_scan_by_job_id(job_id, &scan.sched))) return rc;

    if ((rc = scan_init(num_threads, 10.))) return rc;

    sched_seq_init(&scan.seq);

    int64_t scan_id = scan.sched.id;
    int64_t seq_id = scan.seq.id;
    while (!(rc = api_scan_next_seq(scan_id, seq_id, &scan.seq)))
    {
        struct imm_seq seq = imm_seq(imm_str(scan.seq.data), scan.abc);

        unsigned nparts = profile_reader_npartitions(&scan.profile_reader);
        for (unsigned i = 0; i < nparts; ++i)
        {
            struct scan_thread *t = scan.thread + i;
            thread_setup_seq(t, &seq, scan.seq.id);
            t->job_id = job_id;
        }

        _Pragma ("omp parallel for firstprivate(job_id) schedule(static, 1)")
            for (unsigned i = 0; i < nparts; ++i)
            {
                int tid = xomp_thread_num();
                enum rc local_rc = thread_run(&scan.thread[i], tid);
                if (local_rc)
                {
                    _Pragma ("omp atomic write")
                        rc = local_rc;
                    _Pragma ("omp cancel for")
                }
            }

        if (rc)
        {
            job_set_fail(job_id, "thread_run error (%s)", RC_STRING(rc));
            goto cleanup;
        }
        seq_id = scan.seq.id;
    }

    if (rc == RC_END) return work_finishup(job_id);

    if (rc == RC_EAPI)
        job_set_fail(job_id, api_rc.msg);
    else
        job_set_fail(job_id, "BUG: unexpected return code");

cleanup:
    return rc;
}

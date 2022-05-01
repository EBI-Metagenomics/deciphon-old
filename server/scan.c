#include "scan.h"
#include "deciphon/db/db.h"
#include "deciphon/db/profile_reader.h"
#include "deciphon/info.h"
#include "deciphon/logger.h"
#include "deciphon/rc.h"
#include "deciphon/sched/api.h"
#include "deciphon/sched/sched.h"
#include "deciphon/version.h"
#include "deciphon/xfile.h"
#include "deciphon/xmath.h"
#include "file.h"
#include "imm/imm.h"
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

    char db_filename[FILENAME_SIZE];
    struct protein_db_reader db_reader;
    struct profile_reader profile_reader;

    struct imm_str str;
    struct imm_seq iseq;
    struct imm_abc const *abc;

    prod_fwrite_match_func_t write_match_func;
    double lrt_threshold;
};

static struct scan scan = {0};
static struct api_rc api_rc = {0};
static struct sched_db db = {0};

static inline void fail_job(int64_t job_id, char const *msg)
{
    struct api_rc tmp = {0};
    api_set_job_state(job_id, SCHED_FAIL, msg, &tmp);
}

static enum rc prepare_readers(void)
{
    FILE *fp = fopen(db.filename, "rb");
    if (!fp)
    {
        fail_job(scan.sched.job_id, "failed to open database");
        return eio("failed to open database");
    }

    enum rc rc = protein_db_reader_open(&scan.db_reader, fp);
    if (rc)
    {
        fail_job(scan.sched.job_id, "failed to setup database reader");
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
    enum rc rc = api_download_db(scan.sched.db_id, fp);
    if (rc)
    {
        fail_job(scan.sched.job_id, "failed to download database");
        fclose(fp);
        return rc;
    }
    if (fclose(fp))
    {
        fail_job(scan.sched.job_id, "failed to close database file");
        return eio("fclose");
    }
    return RC_OK;
}

static enum rc scan_init(unsigned num_threads, double lrt_threshold)
{
    scan.num_threads = num_threads;
    scan.lrt_threshold = lrt_threshold;

    enum rc rc = RC_OK;

    if ((rc = prod_fopen(num_threads)))
    {
        fail_job(scan.sched.job_id, "failed to open product files");
        goto cleanup;
    }

    if ((rc = api_get_db(scan.sched.db_id, &db, &api_rc)))
    {
        fail_job(scan.sched.job_id, "failed to get database");
        return rc;
    }
    if (api_rc.rc)
    {
        fail_job(scan.sched.job_id, api_rc.msg);
        return rc;
    }

    if ((rc = file_ensure_local(db.filename, db.xxh3, fetch_db)))
    {
        fail_job(scan.sched.job_id, "failed to have database on disk");
        return rc;
    }

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

cleanup:
    return rc;
}

static enum rc work_finishup(int64_t job_id)
{
    enum rc rc = prod_fclose();
    if (rc)
    {
        fail_job(job_id, "failed to finish up work");
        goto cleanup;
    }

    char const *filepath = prod_final_path();

    if ((rc = api_upload_prods_file(filepath, &api_rc)))
    {
        fail_job(job_id, "failed to submit prods_file");
        goto cleanup;
    }
    if (api_rc.rc)
    {
        rc = erest("failed to submit prods_file");
        fail_job(job_id, api_rc.msg);
        goto cleanup;
    }

    rc = api_set_job_state(job_id, SCHED_DONE, "", &api_rc);
    if (rc) return rc;

    return api_rc.rc ? erest(api_rc.msg) : RC_OK;

cleanup:
    return rc;
}

enum rc scan_run(int64_t job_id, unsigned num_threads)
{
    enum rc rc = RC_OK;

    if ((rc = api_get_scan_by_job_id(job_id, &scan.sched, &api_rc))) return rc;

    if ((rc = scan_init(num_threads, 10.))) return rc;

    sched_seq_init(&scan.seq);

    int nseqs = 0;
    while (!(
        rc = api_scan_next_seq(scan.sched.id, scan.seq.id, &scan.seq, &api_rc)))
    {
        if (api_rc.rc)
        {
            rc = erest(api_rc.msg);
            fail_job(job_id, "failed to fetch new sequence");
            goto cleanup;
        }
        struct imm_seq seq = imm_seq(imm_str(scan.seq.data), scan.abc);

        unsigned npartitions = profile_reader_npartitions(&scan.profile_reader);
        for (unsigned i = 0; i < npartitions; ++i)
        {
            struct scan_thread *t = scan.thread + i;
            thread_setup_seq(t, &seq, scan.seq.id);
        }

#pragma omp parallel for
        for (unsigned i = 0; i < npartitions; ++i)
        {
            struct scan_thread *t = scan.thread + i;
            enum rc local_rc = thread_run(t);
            if (local_rc)
            {
                fail_job(job_id, "internal thread api_rc");
            }
        }
        ++nseqs;
        info("%d of sequences have been scanned", nseqs);
    }
    if (rc == RC_END)
        rc = RC_OK;
    else
        fail_job(job_id, "rc should have been RC_END");

    return work_finishup(job_id);

cleanup:
    return rc;
}

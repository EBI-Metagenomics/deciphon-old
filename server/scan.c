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
static struct api_error error = {0};

static enum rc ensure_database_integrity(char const *filename, int64_t xxh3)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) return eio("fopen");

    int64_t hash = 0;
    enum rc rc = xfile_hash(fp, &hash);
    if (rc)
    {
        fclose(fp);
        return rc;
    }
    fclose(fp);
    return xxh3 == hash ? RC_OK : einval("wrong hash");
}

static enum rc download_database(struct sched_db *db)
{
    FILE *fp = fopen(db->filename, "wb");
    enum rc rc = api_download_db(db->id, fp);
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
        rc = ensure_database_integrity(db->filename, db->xxh3);
        if (!rc) return rc;
    }

    rc = download_database(db);
    if (rc) return rc;

    return ensure_database_integrity(db->filename, db->xxh3);
}

static inline void fail_job(int64_t job_id, char const *msg)
{
    struct api_error rerr = {0};
    api_set_job_state(job_id, SCHED_FAIL, msg, &rerr);
}

static enum rc prepare_database(struct scan *scan)
{
    struct sched_db db = {0};
    db.id = scan->sched.db_id;

    struct api_error rerr = {0};
    enum rc rc = api_get_db(scan->sched.db_id, &db, &rerr);
    if (rc || rerr.rc)
    {
        if (rerr.rc) rc = erest(rerr.msg);
        fail_job(scan->sched.job_id, rerr.msg);
        return rc;
    }
    rc = ensure_database(&db);
    if (rc)
    {
        fail_job(scan->sched.job_id, "failed to ensure database");
        return rc;
    }
    strcpy(scan->db_filename, db.filename);

    return RC_OK;
}

static enum rc prepare_readers(struct scan *scan)
{
    FILE *fp = fopen(scan->db_filename, "rb");
    if (!fp)
    {
        fail_job(scan->sched.job_id, "failed to open database");
        return eio("failed to open database");
    }

    enum rc rc = protein_db_reader_open(&scan->db_reader, fp);
    if (rc)
    {
        fail_job(scan->sched.job_id, "failed to setup database reader");
        goto cleanup;
    }

    struct profile_reader *profile_reader = &scan->profile_reader;
    struct db_reader *db_reader = (struct db_reader *)&scan->db_reader;

    rc = profile_reader_setup(profile_reader, db_reader, scan->num_threads);
    if (rc) goto cleanup;

    scan->abc = (struct imm_abc const *)&scan->db_reader.nuclt;

    return RC_OK;

cleanup:
    if (fp) fclose(fp);
    return rc;
}

static enum rc scan_init(struct scan *scan, unsigned num_threads,
                         double lrt_threshold)
{
    scan->num_threads = num_threads;
    scan->lrt_threshold = lrt_threshold;

    enum rc rc = prod_fopen(num_threads);
    if (rc)
    {
        fail_job(scan->sched.job_id, "failed to open product files");
        goto cleanup;
    }

    rc = prepare_database(scan);
    if (rc)
    {
        prod_fcleanup();
        goto cleanup;
    }

    rc = prepare_readers(scan);
    if (rc)
    {
        prod_fcleanup();
        goto cleanup;
    }
    scan->write_match_func = protein_match_write_func;

    unsigned npartitions = profile_reader_npartitions(&scan->profile_reader);
    for (unsigned i = 0; i < npartitions; ++i)
    {
        struct scan_thread *t = scan->thread + i;
        struct profile_reader *reader = &scan->profile_reader;
        bool multi_hits = scan->sched.multi_hits;
        bool hmmer3_compat = scan->sched.hmmer3_compat;
        imm_float lrt = scan->lrt_threshold;
        prod_fwrite_match_func_t func = scan->write_match_func;

        thread_init(t, i, reader, multi_hits, hmmer3_compat, lrt, func);

        enum imm_abc_typeid abc_typeid = scan->abc->vtable.typeid;
        enum profile_typeid profile_typeid = reader->profile_typeid;
        thread_setup_job(t, abc_typeid, profile_typeid, scan->sched.id);
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

    struct api_error rerr = {0};

    if ((rc = api_upload_prods_file(filepath, &rerr)))
    {
        fail_job(job_id, "failed to submit prods_file");
        goto cleanup;
    }
    if (rerr.rc)
    {
        rc = erest("failed to submit prods_file");
        fail_job(job_id, rerr.msg);
        goto cleanup;
    }

    rc = api_set_job_state(job_id, SCHED_DONE, "", &rerr);
    if (rc) return rc;

    return rerr.rc ? erest(rerr.msg) : RC_OK;

cleanup:
    return rc;
}

enum rc scan_run(int64_t job_id, unsigned num_threads)
{
    enum rc rc = api_get_scan_by_job_id(job_id, &scan.sched, &error);
    if (rc) return rc;

    rc = scan_init(&scan, num_threads, 10.);
    if (rc) return rc;

    sched_seq_init(&scan.seq);

    struct api_error rerr = {0};
    while (
        !(rc = api_scan_next_seq(scan.sched.id, scan.seq.id, &scan.seq, &rerr)))
    {
        if (rerr.rc)
        {
            rc = erest(rerr.msg);
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

        // #pragma omp parallel for
        for (unsigned i = 0; i < npartitions; ++i)
        {
            struct scan_thread *t = scan.thread + i;
            enum rc local_rc = thread_run(t);
            if (local_rc)
            {
                fail_job(job_id, "internal thread error");
            }
        }
    }
    if (rc != RC_END) goto cleanup;

    return work_finishup(job_id);

cleanup:
    return rc;
}

enum rc scan_cleanup(struct scan *scan)
{
    (void)scan;
    return RC_OK;
}

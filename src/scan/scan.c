#include "scan/scan.h"
#include "core/c23.h"
#include "core/logging.h"
#include "db/profile_reader.h"
#include "db/protein_reader.h"
#include "jx.h"
#include "scan/prod.h"
#include "scan/seqs.h"
#include "scan/thread.h"
#include "xfile.h"
#include "zc.h"
#include <stdlib.h>

struct scan
{
    int64_t scan_id;

    struct scan_cfg cfg;
    struct scan_thread thread[NUM_THREADS];

    struct seqs seqs;

    struct protein_db_reader db_reader;
    struct profile_reader profile_reader;

    struct imm_str str;
    struct imm_seq iseq;
    struct imm_abc const *abc;

    char job_error[SCHED_JOB_ERROR_SIZE];

    prod_fwrite_match_func_t write_match_func;
};

static struct scan scan = {0};
static char const *fail_job(char const *msg);
static enum rc prepare_readers(char const *db);
static void scan_cleanup(void);

void scan_init(struct scan_cfg cfg) { scan.cfg = cfg; }

enum rc scan_setup(char const *db, char const *seqs)
{
    enum rc rc = RC_OK;

    info("Reading seqs");
    if ((rc = seqs_init(&scan.seqs, seqs)))
    {
        fail_job("failed to read seqs file");
        return rc;
    }

    if ((rc = prod_fopen(scan.cfg.num_threads)))
    {
        fail_job("failed to open product files");
        scan_cleanup();
        return rc;
    }

    info("Preparing readers");
    if ((rc = prepare_readers(db)))
    {
        scan_cleanup();
        prod_fcleanup();
        return rc;
    }
    scan.write_match_func = protein_match_write_func;

    unsigned npartitions = profile_reader_npartitions(&scan.profile_reader);
    for (unsigned i = 0; i < npartitions; ++i)
    {
        struct scan_thread *t = scan.thread + i;
        struct profile_reader *reader = &scan.profile_reader;
        prod_fwrite_match_func_t func = scan.write_match_func;

        thread_init(t, i, reader, scan.cfg, func);

        enum imm_abc_typeid abc_typeid = scan.abc->vtable.typeid;
        enum profile_typeid profile_typeid = reader->profile_typeid;
        thread_setup_job(t, abc_typeid, profile_typeid, scan.scan_id);
    }

    unsigned long total =
        scan.seqs.size * profile_reader_nprofiles(&scan.profile_reader);
    info("%lu tasks to run", total);

    return rc;
}

#if 0
enum rc scan_run(void)
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
#endif

static char const *fail_job(char const *msg)
{
    zc_strlcpy(scan.job_error, msg, SCHED_JOB_ERROR_SIZE);
    return msg;
}

static enum rc prepare_readers(char const *db)
{
    FILE *fp = fopen(db, "rb");
    if (!fp)
    {
        return eio(fail_job("failed to open database"));
    }

    enum rc rc = protein_db_reader_open(&scan.db_reader, fp);
    if (rc)
    {
        fail_job("failed to setup database reader");
        goto cleanup;
    }

    struct profile_reader *profile_reader = &scan.profile_reader;
    struct db_reader *db_reader = (struct db_reader *)&scan.db_reader;

    rc = profile_reader_setup(profile_reader, db_reader, scan.cfg.num_threads);
    if (rc) goto cleanup;

    scan.abc = (struct imm_abc const *)&scan.db_reader.nuclt;

    return RC_OK;

cleanup:
    if (fp) fclose(fp);
    return rc;
}

static void scan_cleanup(void) { seqs_cleanup(&scan.seqs); }

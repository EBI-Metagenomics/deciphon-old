#include "scan/scan.h"
#include "core/errmsg.h"
#include "core/progress.h"
#include "db/profile_reader.h"
#include "db/protein_reader.h"
#include "jx.h"
#include "logy.h"
#include "scan/prod.h"
#include "scan/prodman.h"
#include "scan/seq.h"
#include "scan/seqlist.h"
#include "scan/thread.h"
#include "zc.h"
#include <stdlib.h>

static struct scan_cfg scan_cfg = {0};

static struct thread thread[NUM_THREADS] = {0};

static FILE *db_file = NULL;
static struct protein_db_reader pro_db_reader = {0};
static struct db_reader *db_reader = (struct db_reader *)&pro_db_reader;

static struct profile_reader profreader = {0};

static struct imm_abc const *abc = (struct imm_abc const *)&pro_db_reader.nuclt;

static enum rc errnum = RC_OK;
static char errmsg[ERROR_SIZE] = {0};

static struct progress progress = {0};

static enum rc prepare_readers(char const *db);

void scan_init(struct scan_cfg cfg)
{
    scan_cfg = cfg;
    errnum = RC_OK;
    errmsg[0] = '\0';
    progress_init(&progress, 0);
    prodman_init();
}

int scan_setup(char const *db, char const *seqs)
{
    if ((errnum = prodman_setup(scan_cfg.nthreads)))
    {
        errfmt(errmsg, "failed to open product files");
        scan_cleanup();
        return errnum;
    }

    info("preparing readers...");
    if ((errnum = prepare_readers(db)))
    {
        scan_cleanup();
        return errnum;
    }

    info("reading sequences...");
    if ((errnum = seqlist_init(seqs, abc)))
    {
        errfmt(errmsg, "%s", seqlist_errmsg());
        scan_cleanup();
        return errnum;
    }

    long ntasks = 0;
    long ntasks_total = 0;
    unsigned nparts = profile_reader_npartitions(&profreader);
    for (unsigned i = 0; i < nparts; ++i)
    {
        struct thread *t = thread + i;
        struct profile_reader *reader = &profreader;
        prod_fwrite_match_fn_t *cb = &protein_match_write;

        thread_init(t, prodman_file(i), i, reader, scan_cfg, cb);

        enum imm_abc_typeid abc_typeid = abc->vtable.typeid;
        enum profile_typeid prof_typeid = reader->profile_typeid;
        ntasks = profile_reader_partition_size(&profreader, i) * seqlist_size();
        assert(ntasks > 0);
        thread_setup_job(t, abc_typeid, prof_typeid, seqlist_scan_id(), ntasks);

        ntasks_total += ntasks;
    }

    progress_init(&progress, ntasks_total);
    info("%ld tasks to run", ntasks_total);

    return errnum;
}

int scan_run(void)
{
    struct seq const *seq = NULL;
    enum rc rc[NUM_THREADS] = {0};
    unsigned last_tid = UINT_MAX;
    seqlist_rewind();
    while ((seq = seqlist_next()))
    {
        struct imm_seq const *iseq = seq_iseq(seq);

        unsigned nparts = profile_reader_npartitions(&profreader);
        for (unsigned i = 0; i < nparts; ++i)
            thread_setup_seq(thread + i, iseq, seq->id);

        _Pragma ("omp parallel for schedule(static, 1)")
            for (unsigned i = 0; i < nparts; ++i)
            {
                if ((rc[i] = thread_run(&thread[i])))
                {
                    _Pragma ("omp atomic write")
                        last_tid = i;
                    _Pragma ("omp cancel for")
                }
            }
        if (last_tid != UINT_MAX)
        {
            errnum = rc[last_tid];
            errfmt(errmsg, "%s", thread_errmsg(&thread[last_tid]));
            break;
        }
    }

    return errnum;
}

int scan_progress_update(void)
{
    unsigned nparts = profile_reader_npartitions(&profreader);
    long prev = progress_consumed(&progress);
    long now = 0;
    for (unsigned i = 0; i < nparts; ++i)
    {
        struct progress const *p = thread_progress(thread + i);
        now += progress_consumed(p);
    }
    return progress_consume(&progress, now - prev);
}

struct progress const *scan_progress(void) { return &progress; }

char const *scan_errmsg(void) { return errmsg; }

int scan_finishup(char const **filepath) { return prodman_finishup(filepath); }

void scan_cleanup(void)
{
    seqlist_cleanup();
    if (db_file)
    {
        fclose(db_file);
        db_file = NULL;
    }
    prodman_cleanup();
}

static enum rc prepare_readers(char const *db)
{
    db_file = fopen(db, "rb");
    if (!db_file) return eio("%s", errfmt(errmsg, "failed to open database"));

    enum rc rc = protein_db_reader_open(&pro_db_reader, db_file);
    if (rc)
    {
        errfmt(errmsg, "failed to setup database reader");
        goto cleanup;
    }

    rc = profile_reader_setup(&profreader, db_reader, scan_cfg.nthreads);
    if (rc)
    {
        errfmt(errmsg, "failed to setup profile reader");
        goto cleanup;
    }

cleanup:
    return rc;
}

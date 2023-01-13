#include "work.h"
#include "atomic.h"
#include "db/profile_reader.h"
#include "db/prot_reader.h"
#include "deciphon_limits.h"
#include "filename.h"
#include "fs.h"
#include "hmmer/client.h"
#include "hmmer/server.h"
#include "hmmer/state.h"
#include "logy.h"
#include "loop/global.h"
#include "loop/machine.h"
#include "loop/now.h"
#include "loop_while.h"
#include "progress.h"
#include "scan/scan.h"
#include "state.h"
#include "strlcpy.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <uv.h>

static enum state state = INIT;
static atomic_bool cancel = false;
static struct uv_work_s work_request = {0};
static struct progress progress = {0};

static int nthreads = 0;
static char seqsfile[FILENAME_SIZE] = {0};
static char dbfile[FILENAME_SIZE] = {0};
static char hmmfile[FILENAME_SIZE] = {0};

void work_init(void)
{
    nthreads = machine_ncpus() > 1 ? machine_ncpus() - 1 : 1;
    state = INIT;
    cancel = false;
    progress_init(&progress, 0);
}

enum state work_state(void) { return state; }

int work_progress(void) { return progress_percent(&progress); }

static void run(struct uv_work_s *);
static void end(struct uv_work_s *, int status);

int work_run(char const *seqs, char const *db, bool multi_hits,
             bool hmmer3_compat)
{
    if (state == RUN) return einval("work thread is busy");
    assert(!cancel);

    if (strlcpy(seqsfile, seqs, sizeof(seqsfile)) >= sizeof(seqsfile))
        return enomem("file name is too long");

    if (strlcpy(dbfile, db, sizeof(dbfile)) >= sizeof(dbfile))
        return enomem("file name is too long");

    struct scan_cfg cfg = {nthreads, 10., multi_hits, hmmer3_compat};
    scan_init(cfg);

    strcpy(hmmfile, dbfile);
    filename_setext(hmmfile, "hmm");
    info("hmmfile: %s", hmmfile);
    info("dbfile: %s", dbfile);

    if (uv_queue_work(global_loop(), &work_request, &run, &end))
        return efail("failed to queue work");

    progress_init(&progress, 0);
    state = RUN;
    return RC_OK;
}

char const *work_seqsfile(void) { return seqsfile; }

void work_cancel(void)
{
    if (state != RUN) return;
    uv_cancel((struct uv_req_s *)&work_request);
    atomic_release(&cancel, true);
}

static void run(struct uv_work_s *w)
{
    (void)w;
    info("starting scan with %d threads", nthreads);
    info("hmmer_server_start: %d", hmmer_server_start(hmmfile));
    loop_while(40000, hmmer_server_state() == HMMERD_BOOT);
    info("hmmer_server_state: %d", hmmer_server_state());
    info("hmmer_client_start: %d", hmmer_client_start(nthreads, now() + 5000));

    int rc = scan_setup(dbfile, seqsfile);
    if (rc)
    {
        efail("%s", scan_errmsg());
        state = FAIL;
        goto cleanup;
    }

    if ((rc = scan_run()))
    {
        efail("%s", scan_errmsg());
        state = FAIL;
        goto cleanup;
    }

    if (atomic_consume(&cancel))
    {
        info("cancelled");
        state = CANCEL;
        goto cleanup;
    }

    if ((rc = scan_finishup()))
    {
        efail("%s", scan_errmsg());
        state = FAIL;
        goto cleanup;
    }

    state = DONE;
    info("scan has finished");

cleanup:
    info("end 1");
    scan_cleanup();
    info("end 2");
    hmmer_client_stop();
    info("end 3");
    hmmer_server_stop();
    info("end 4");
    loop_while(15000, hmmer_server_state() == HMMERD_ON);
    info("end 5");
    hmmer_server_close();
}

static void end(struct uv_work_s *w, int status)
{
    (void)w;
    if (status == UV_ECANCELED && state == RUN) state = CANCEL;
    atomic_release(&cancel, false);
    assert(state == CANCEL || state == DONE || state == FAIL);
}
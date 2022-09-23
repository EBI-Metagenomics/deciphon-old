#include "scanny_session.h"
#include "core/errmsg.h"
#include "core/limits.h"
#include "core/logging.h"
#include "core/progress.h"
#include "db/profile_reader.h"
#include "db/protein_reader.h"
#include "jx.h"
#include "scan/scan.h"
#include "scan/thread.h"
#include "uv.h"
#include "xfile.h"
#include "zc.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>

enum state
{
    IDLE,
    RUN,
    DONE,
    FAIL,
    CANCEL,
};

static struct
{
    struct uv_loop_s *loop;
    char seqs[PATH_SIZE];
    char db[PATH_SIZE];
    char prod[PATH_SIZE];

    unsigned num_threads;
    double lrt_threshold;

    atomic_bool cancel;
    enum state state;
    struct uv_work_s request;
    struct progress progress;
} session;

static enum rc errnum = RC_OK;
static char errmsg[ERROR_SIZE] = {0};

static char const *state_string[] = {[IDLE] = "IDLE",
                                     [RUN] = "RUN",
                                     [DONE] = "DONE",
                                     [FAIL] = "FAIL",
                                     [CANCEL] = "CANCEL"};

static void after_work(struct uv_work_s *, int status);
static void work(struct uv_work_s *);

void scanny_session_init(struct uv_loop_s *loop)
{
    session.loop = loop;
    session.cancel = false;
    session.state = IDLE;
    errnum = RC_OK;
    errmsg[0] = '\0';
    progress_init(&session.progress, 0);
}

bool scanny_session_is_running(void) { return session.state == RUN; }

bool scanny_session_is_done(void) { return session.state == DONE; }

bool scanny_session_start(char const *seqs, char const *db, char const *prod,
                          bool multi_hits, bool hmmer3_compat)
{
    errnum = RC_OK;
    errmsg[0] = '\0';
    session.cancel = false;
    session.state = RUN;

    if (zc_strlcpy(session.seqs, seqs, PATH_SIZE) >= PATH_SIZE)
    {
        session.state = FAIL;
        return !(errnum = enomem(errfmt(errmsg, "file path is too long")));
    }

    if (zc_strlcpy(session.db, db, PATH_SIZE) >= PATH_SIZE)
    {
        session.state = FAIL;
        return !(errnum = enomem(errfmt(errmsg, "file path is too long")));
    }

    if (zc_strlcpy(session.prod, prod, PATH_SIZE) >= PATH_SIZE)
    {
        session.state = FAIL;
        return !(errnum = enomem(errfmt(errmsg, "file path is too long")));
    }

    struct scan_cfg cfg = {1, 10., multi_hits, hmmer3_compat};
    scan_init(cfg);

    uv_queue_work(session.loop, &session.request, work, after_work);

    return true;
}

unsigned scanny_session_progress(void)
{
    return progress_percent(&session.progress);
}

bool scanny_session_cancel(void)
{
    info("Cancelling...");
    if (atomic_load(&session.cancel))
    {
        int rc = uv_cancel((struct uv_req_s *)&session.request);
        if (rc)
        {
            warn(uv_strerror(rc));
            return false;
        }
        return true;
    }
    atomic_store(&session.cancel, true);
    return true;
}

char const *scanny_session_state_string(void)
{
    return state_string[session.state];
}

static void after_work(struct uv_work_s *req, int status)
{
    (void)req;
    if (status == UV_ECANCELED) session.state = CANCEL;
    atomic_store(&session.cancel, false);
}

static void work(struct uv_work_s *req)
{
    (void)req;
    info("Preparing to scan");

    errnum = scan_setup(session.db, session.seqs);
    if (errnum)
    {
        errfmt(errmsg, "%s", scan_errmsg());
        session.state = FAIL;
        return;
    }

    if (atomic_load(&session.cancel))
    {
        info("Cancelled");
        session.state = CANCEL;
        return;
    }

    if ((errnum = scan_run()))
    {
        errfmt(errmsg, "%s", scan_errmsg());
        session.state = FAIL;
        return;
    }

    if (atomic_load(&session.cancel))
    {
        info("Cancelled");
        session.state = CANCEL;
        return;
    }

    int r = xfile_move(session.prod, scan_prod_filepath());
    if (r)
    {
        errnum = eio(errfmt(errmsg, "%s", xfile_strerror(r)));
        session.state = FAIL;
        return;
    }

    session.state = DONE;
}

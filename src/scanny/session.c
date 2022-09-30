#include "scanny/session.h"
#include "core/errmsg.h"
#include "core/limits.h"
#include "core/logy.h"
#include "core/progress.h"
#include "core/strings.h"
#include "db/profile_reader.h"
#include "db/protein_reader.h"
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

    int nthreads;
    double lrt_threshold;

    atomic_bool cancel;
    enum state state;
    struct uv_work_s request;
    struct progress progress;
} session;

struct uv_timer_s monitor_timer = {0};
atomic_bool nomonitor = true;

static enum rc errnum = RC_OK;
static char errmsg[ERROR_SIZE] = {0};

static char const *state_string[] = {[IDLE] = "IDLE",
                                     [RUN] = "RUN",
                                     [DONE] = "DONE",
                                     [FAIL] = "FAIL",
                                     [CANCEL] = "CANCEL"};

static void after_work(struct uv_work_s *, int status);
static void work(struct uv_work_s *);

static void monitor_start(void);
static struct progress const *monitor_progress(void);
static void monitor_progress_cb(struct uv_timer_s *);
static void monitor_stop(void);

void session_init(struct uv_loop_s *loop)
{
    session.loop = loop;
    session.nthreads = 1;
    session.cancel = false;
    session.state = IDLE;
    errnum = RC_OK;
    errmsg[0] = '\0';
    progress_init(&session.progress, 0);
}

void session_set_nthreads(int num_threads) { session.nthreads = num_threads; }

bool session_is_running(void) { return session.state == RUN; }

bool session_is_done(void) { return session.state == DONE; }

bool session_start(char const *seqs, char const *db, char const *prod,
                   bool multi_hits, bool hmmer3_compat)
{
    errnum = RC_OK;
    errmsg[0] = '\0';
    session.cancel = false;
    session.state = RUN;

    if (zc_strlcpy(session.seqs, seqs, PATH_SIZE) >= PATH_SIZE)
    {
        session.state = FAIL;
        return !(errnum = enomem("%s", errfmt(errmsg, FILE_PATH_LONG)));
    }

    if (zc_strlcpy(session.db, db, PATH_SIZE) >= PATH_SIZE)
    {
        session.state = FAIL;
        return !(errnum = enomem("%s", errfmt(errmsg, FILE_PATH_LONG)));
    }

    if (zc_strlcpy(session.prod, prod, PATH_SIZE) >= PATH_SIZE)
    {
        session.state = FAIL;
        return !(errnum = enomem("%s", errfmt(errmsg, FILE_PATH_LONG)));
    }

    struct scan_cfg cfg = {session.nthreads, 10., multi_hits, hmmer3_compat};
    scan_init(cfg);

    monitor_start();
    uv_queue_work(session.loop, &session.request, work, after_work);

    return true;
}

int session_progress(void)
{
    struct progress const *p = monitor_progress();
    if (p) return (unsigned)progress_percent(p);
    return -1;
}

bool session_cancel(void)
{
    info("Cancelling...");
    if (atomic_load(&session.cancel))
    {
        int rc = uv_cancel((struct uv_req_s *)&session.request);
        if (rc)
        {
            warn("%s", uv_strerror(rc));
            return false;
        }
        return true;
    }
    atomic_store(&session.cancel, true);
    return true;
}

char const *session_state_string(void) { return state_string[session.state]; }

static void after_work(struct uv_work_s *req, int status)
{
    (void)req;
    if (status == UV_ECANCELED) session.state = CANCEL;
    atomic_store(&session.cancel, false);
    atomic_store_explicit(&nomonitor, true, memory_order_release);
    monitor_stop();
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
    atomic_store_explicit(&nomonitor, false, memory_order_release);

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
        errnum = eio("%s", errfmt(errmsg, "%s", xfile_strerror(r)));
        session.state = FAIL;
        return;
    }

    session.state = DONE;
}

static void monitor_start(void)
{
    if (uv_timer_init(session.loop, &monitor_timer)) efail("uv_timer_init");
    if (uv_timer_start(&monitor_timer, &monitor_progress_cb, 1000, 1000))
        efail("uv_timer_start");
}

static struct progress const *monitor_progress(void)
{
    if (atomic_load_explicit(&nomonitor, memory_order_consume)) return NULL;
    scan_progress_update();
    return scan_progress();
}

static void monitor_progress_cb(struct uv_timer_s *req)
{
    (void)req;
    if (atomic_load_explicit(&nomonitor, memory_order_consume)) return;
    if (scan_progress_update())
    {
        struct progress const *p = scan_progress();
        info("Scanned %d%%", progress_percent(p));
    }
}

static void monitor_stop(void)
{
    uv_timer_stop(&monitor_timer);
    if (session.state == DONE) info("Scanned %d%%", 100);
}

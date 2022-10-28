#include "scanner.h"
#include "core/errmsg.h"
#include "core/global.h"
#include "core/limits.h"
#include "core/logy.h"
#include "core/machine.h"
#include "core/progress.h"
#include "core/strings.h"
#include "db/profile_reader.h"
#include "db/protein_reader.h"
#include "fs.h"
#include "scan/scan.h"
#include "scan/thread.h"
#include "uv.h"
#include "zc.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>

enum state
{
    STATE_IDLE,
    STATE_RUN,
    STATE_DONE,
    STATE_FAIL,
    STATE_CANCEL,
};

static struct
{
    char seqs[PATH_SIZE];
    char db[PATH_SIZE];
    char prod[PATH_SIZE];

    int nthreads;
    double lrt_threshold;

    atomic_bool cancel;
    enum state state;
    struct uv_work_s request;
    struct progress progress;
} self;

struct uv_timer_s monitor_timer = {0};
atomic_bool no_monitor = true;

static enum rc errnum = RC_OK;
static char errmsg[ERROR_SIZE] = {0};

static char const *state_string[] = {[STATE_IDLE] = "idle",
                                     [STATE_RUN] = "run",
                                     [STATE_DONE] = "done",
                                     [STATE_FAIL] = "fail",
                                     [STATE_CANCEL] = "cancel"};

static void after_work(struct uv_work_s *, int status);
static void work(struct uv_work_s *);

static void monitor_start(void);
static struct progress const *monitor_progress(void);
static void monitor_progress_cb(struct uv_timer_s *);
static void monitor_stop(void);

void scanner_init(void)
{
    self.nthreads = machine_ncpus() > 1 ? machine_ncpus() - 1 : 1;
    self.cancel = false;
    self.state = STATE_IDLE;
    errnum = RC_OK;
    errmsg[0] = '\0';
    progress_init(&self.progress, 0);
}

void scanner_reset(void)
{
    if (scanner_is_running()) return;
    self.cancel = false;
    self.state = STATE_IDLE;
    errnum = RC_OK;
    errmsg[0] = '\0';
    progress_init(&self.progress, 0);
}

char const *scanner_filename(void) { return self.prod; }

void scanner_set_nthreads(int num_threads) { self.nthreads = num_threads; }

bool scanner_is_running(void) { return self.state == STATE_RUN; }

bool scanner_is_done(void) { return self.state == STATE_DONE; }

bool scanner_start(char const *seqs, char const *db, char const *prod,
                   bool multi_hits, bool hmmer3_compat)
{
    info("starting scan with %d threads", self.nthreads);
    errnum = RC_OK;
    errmsg[0] = '\0';
    atomic_store_explicit(&self.cancel, false, memory_order_release);
    self.state = STATE_RUN;

    if (zc_strlcpy(self.seqs, seqs, PATH_SIZE) >= PATH_SIZE)
    {
        self.state = STATE_FAIL;
        return !(errnum = enomem("%s", errfmt(errmsg, FILE_PATH_LONG)));
    }

    if (zc_strlcpy(self.db, db, PATH_SIZE) >= PATH_SIZE)
    {
        self.state = STATE_FAIL;
        return !(errnum = enomem("%s", errfmt(errmsg, FILE_PATH_LONG)));
    }

    if (zc_strlcpy(self.prod, prod, PATH_SIZE) >= PATH_SIZE)
    {
        self.state = STATE_FAIL;
        return !(errnum = enomem("%s", errfmt(errmsg, FILE_PATH_LONG)));
    }

    struct scan_cfg cfg = {self.nthreads, 10., multi_hits, hmmer3_compat};
    scan_init(cfg);

    monitor_start();
    uv_queue_work(global_loop(), &self.request, work, after_work);

    return true;
}

int scanner_progress(void)
{
    struct progress const *p = monitor_progress();
    return p ? progress_percent(p) : 0;
}

int scanner_cancel(int timeout_msec)
{
    info("Cancelling...");
    if (!scanner_is_running()) return RC_OK;

    if (atomic_load_explicit(&self.cancel, memory_order_consume))
    {
        int rc = uv_cancel((struct uv_req_s *)&self.request);
        if (rc)
        {
            warn("%s", uv_strerror(rc));
            return RC_EFAIL;
        }
        return RC_OK;
    }
    atomic_store_explicit(&self.cancel, true, memory_order_release);
    if (timeout_msec > 0)
    {
        uv_sleep(timeout_msec);
        if (scanner_is_running())
        {
            warn("scanner cancelling timed out");
            return RC_TIMEDOUT;
        }
    }
    return RC_OK;
}

char const *scanner_state_string(void) { return state_string[self.state]; }

static void after_work(struct uv_work_s *req, int status)
{
    (void)req;
    if (status == UV_ECANCELED) self.state = STATE_CANCEL;
    atomic_store_explicit(&self.cancel, false, memory_order_release);
    atomic_store_explicit(&no_monitor, true, memory_order_release);
    monitor_stop();
}

static void work(struct uv_work_s *req)
{
    (void)req;

    info("scan setup...");
    errnum = scan_setup(self.db, self.seqs);
    if (errnum)
    {
        errfmt(errmsg, "%s", scan_errmsg());
        self.state = STATE_FAIL;
        scan_cleanup();
        return;
    }
    atomic_store_explicit(&no_monitor, false, memory_order_release);

    if (atomic_load_explicit(&self.cancel, memory_order_consume))
    {
        info("Cancelled");
        self.state = STATE_CANCEL;
        scan_cleanup();
        return;
    }

    if ((errnum = scan_run()))
    {
        errfmt(errmsg, "%s", scan_errmsg());
        self.state = STATE_FAIL;
        scan_cleanup();
        return;
    }

    if (atomic_load_explicit(&self.cancel, memory_order_consume))
    {
        info("Cancelled");
        self.state = STATE_CANCEL;
        scan_cleanup();
        return;
    }

    char const *filepath = NULL;
    if ((errnum = scan_finishup(&filepath)))
    {
        errfmt(errmsg, "%s", scan_errmsg());
        self.state = STATE_FAIL;
        scan_cleanup();
        return;
    }

    int r = fs_move(self.prod, filepath);
    if (r)
    {
        errnum = eio("%s", errfmt(errmsg, "%s", fs_strerror(r)));
        self.state = STATE_FAIL;
        scan_cleanup();
        return;
    }

    self.state = STATE_DONE;
    scan_cleanup();
}

static void monitor_start(void)
{
    if (uv_timer_init(global_loop(), &monitor_timer)) efail("uv_timer_init");
    if (uv_timer_start(&monitor_timer, &monitor_progress_cb, 1000, 1000))
        efail("uv_timer_start");
}

static struct progress const *monitor_progress(void)
{
    if (atomic_load_explicit(&no_monitor, memory_order_consume)) return NULL;
    scan_progress_update();
    return scan_progress();
}

static void monitor_progress_cb(struct uv_timer_s *req)
{
    (void)req;
    if (atomic_load_explicit(&no_monitor, memory_order_consume)) return;
    if (scan_progress_update())
    {
        struct progress const *p = scan_progress();
        info("Scanned %d%%", progress_percent(p));
    }
}

static void monitor_stop(void)
{
    uv_timer_stop(&monitor_timer);
    if (self.state == STATE_DONE) info("Scanned %d%%", 100);
}

#include "presser.h"
#include "core/global.h"
#include "core/logy.h"
#include "core/progress.h"
#include "db/press.h"
#include "uv.h"
#include "zc.h"
#include <stdatomic.h>
#include <stdbool.h>

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
    char hmm[PATH_SIZE];
    char db[PATH_SIZE];
    atomic_bool cancel;
    enum state state;
    struct db_press db_press;
    struct uv_work_s request;
    struct progress progress;
    int last_progress;
} self;

static char const *state_string[] = {[IDLE] = "idle",
                                     [RUN] = "run",
                                     [DONE] = "done",
                                     [FAIL] = "fail",
                                     [CANCEL] = "cancel"};

static void after_work(struct uv_work_s *, int status);
static void work(struct uv_work_s *);
static void reset(void)
{
    self.cancel = false;
    self.state = IDLE;
    progress_init(&self.progress, 0);
}

void presser_init(void) { reset(); }

void presser_reset(void)
{
    if (presser_is_running()) return;
    reset();
}

bool presser_is_running(void) { return self.state == RUN; }

bool presser_is_done(void) { return self.state == DONE; }

bool presser_start(char const *hmm)
{
    if (zc_strlcpy(self.hmm, hmm, PATH_SIZE) >= PATH_SIZE)
    {
        enomem("file path is too long");
        return false;
    }

    self.state = RUN;

    strcpy(self.db, self.hmm);
    self.db[strlen(self.db) - 3] = 'd';
    self.db[strlen(self.db) - 2] = 'c';
    self.db[strlen(self.db) - 1] = 'p';

    atomic_store_explicit(&self.cancel, false, memory_order_release);
    uv_queue_work(global_loop(), &self.request, &work, &after_work);
    self.last_progress = 0;

    return true;
}

char const *presser_filename(void) { return self.hmm; }

int presser_progress(void)
{
    return presser_is_done() ? 100 : progress_percent(&self.progress);
}

int presser_inc_progress(void)
{
    if (!(presser_is_running() || presser_is_done())) return 0;
    int progress = progress_percent(&self.progress);
    int inc = progress - self.last_progress;
    self.last_progress = progress;
    return inc;
}

int presser_cancel(int timeout_msec)
{
    info("cancelling...");
    if (!presser_is_running()) return RC_OK;

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
        if (presser_is_running())
        {
            warn("presser cancelling timed out");
            return RC_TIMEDOUT;
        }
    }
    return RC_OK;
}

char const *presser_state_string(void) { return state_string[self.state]; }

void presser_cleanup(void) {}

static void after_work(struct uv_work_s *req, int status)
{
    (void)req;
    if (status == UV_ECANCELED) self.state = CANCEL;
    atomic_store_explicit(&self.cancel, false, memory_order_release);
}

static void work(struct uv_work_s *req)
{
    (void)req;
    enum rc rc = db_press_init(&self.db_press, self.hmm, self.db);
    if (rc)
    {
        self.state = FAIL;
        return;
    }

    progress_init(&self.progress, (long)db_press_nsteps(&self.db_press));
    info("found %u profiles. Pressing...", db_press_nsteps(&self.db_press));
    while (!(rc = db_press_step(&self.db_press)))
    {
        if (atomic_load_explicit(&self.cancel, memory_order_consume))
        {
            info("cancelled");
            self.state = CANCEL;
            db_press_cleanup(&self.db_press, false);
            return;
        }

        progress_consume(&self.progress, 1);
        // if (progress_consume(&self.progress, 1))
        //     info("Pressed %d%% of profiles",
        //     progress_percent(&self.progress));
    }

    if (rc != RC_END)
    {
        self.state = FAIL;
        db_press_cleanup(&self.db_press, false);
        return;
    }

    if ((rc = db_press_cleanup(&self.db_press, true)))
    {
        self.state = FAIL;
        return;
    }
    self.state = DONE;
    info("pressing has finished");
}

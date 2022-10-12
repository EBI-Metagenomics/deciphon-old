#include "session.h"
#include "core/c23.h"
#include "core/global.h"
#include "core/logy.h"
#include "core/progress.h"
#include "db/press.h"
#include "uv.h"
#include "xfile.h"
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
} self;

static char const *state_string[] = {[IDLE] = "IDLE",
                                     [RUN] = "RUN",
                                     [DONE] = "DONE",
                                     [FAIL] = "FAIL",
                                     [CANCEL] = "CANCEL"};

static void after_work(struct uv_work_s *, int status);
static void work(struct uv_work_s *);

void session_init(void)
{
    self.cancel = false;
    atomic_store(&self.cancel, false);
    self.state = IDLE;
    progress_init(&self.progress, 0);
}

bool session_is_running(void) { return self.state == RUN; }

bool session_is_done(void) { return self.state == DONE; }

bool session_start(char const *hmm)
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

    atomic_store(&self.cancel, false);
    uv_queue_work(global_loop(), &self.request, work, after_work);

    return true;
}

unsigned session_progress(void) { return progress_percent(&self.progress); }

bool session_cancel(void)
{
    info("Cancelling...");
    if (atomic_load(&self.cancel))
    {
        int rc = uv_cancel((struct uv_req_s *)&self.request);
        if (rc)
        {
            warn("%s", uv_strerror(rc));
            return false;
        }
        return true;
    }
    atomic_store(&self.cancel, true);
    return true;
}

char const *session_state_string(void) { return state_string[self.state]; }

static void after_work(struct uv_work_s *req, int status)
{
    (void)req;
    if (status == UV_ECANCELED) self.state = CANCEL;
    atomic_store(&self.cancel, false);
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
    info("Found %u profiles. Pressing...", db_press_nsteps(&self.db_press));
    while (!(rc = db_press_step(&self.db_press)))
    {
        if (progress_consume(&self.progress, 1))
        {
            if (atomic_load(&self.cancel))
            {
                info("Cancelled");
                self.state = CANCEL;
                db_press_cleanup(&self.db_press, false);
                return;
            }
            info("Pressed %d%% of profiles", progress_percent(&self.progress));
        }
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
    info("Pressing has finished");
}

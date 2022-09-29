#include "pressy/session.h"
#include "core/c23.h"
#include "core/logging.h"
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
    struct uv_loop_s *loop;
    char hmm[PATH_SIZE];
    char db[PATH_SIZE];
    atomic_bool cancel;
    enum state state;
    struct db_press db_press;
    struct uv_work_s request;
    struct progress progress;
} session;

static char const *state_string[] = {[IDLE] = "IDLE",
                                     [RUN] = "RUN",
                                     [DONE] = "DONE",
                                     [FAIL] = "FAIL",
                                     [CANCEL] = "CANCEL"};

static void after_work(struct uv_work_s *, int status);
static void work(struct uv_work_s *);

void session_init(struct uv_loop_s *loop)
{
    session.loop = loop;
    session.cancel = false;
    atomic_store(&session.cancel, false);
    session.state = IDLE;
    progress_init(&session.progress, 0);
}

bool session_is_running(void) { return session.state == RUN; }

bool session_is_done(void) { return session.state == DONE; }

bool session_start(char const *hmm)
{
    if (zc_strlcpy(session.hmm, hmm, PATH_SIZE) >= PATH_SIZE)
    {
        enomem("file path is too long");
        return false;
    }

    session.state = RUN;

    strcpy(session.db, session.hmm);
    session.db[strlen(session.db) - 3] = 'd';
    session.db[strlen(session.db) - 2] = 'c';
    session.db[strlen(session.db) - 1] = 'p';

    atomic_store(&session.cancel, false);
    uv_queue_work(session.loop, &session.request, work, after_work);

    return true;
}

unsigned session_progress(void) { return progress_percent(&session.progress); }

bool session_cancel(void)
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

char const *session_state_string(void) { return state_string[session.state]; }

static void after_work(struct uv_work_s *req, int status)
{
    (void)req;
    if (status == UV_ECANCELED) session.state = CANCEL;
    atomic_store(&session.cancel, false);
}

static void work(struct uv_work_s *req)
{
    (void)req;
    info("Preparing to press...");
    enum rc rc = db_press_init(&session.db_press, session.hmm, session.db);
    if (rc)
    {
        session.state = FAIL;
        return;
    }

    progress_init(&session.progress, (long)db_press_nsteps(&session.db_press));
    while (!(rc = db_press_step(&session.db_press)))
    {
        if (progress_consume(&session.progress, 1))
        {
            if (atomic_load(&session.cancel))
            {
                info("Cancelled");
                session.state = CANCEL;
                db_press_cleanup(&session.db_press, false);
                return;
            }
            info("Pressed %d%% of profiles",
                 progress_percent(&session.progress));
        }
    }

    if (rc != RC_END)
    {
        session.state = FAIL;
        db_press_cleanup(&session.db_press, false);
        return;
    }

    if ((rc = db_press_cleanup(&session.db_press, true)))
    {
        session.state = FAIL;
        return;
    }
    session.state = DONE;
    info("Pressing has finished");
}

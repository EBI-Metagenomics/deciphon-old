#include "work.h"
#include "core/atomic.h"
#include "core/global.h"
#include "core/logy.h"
#include "core/progress.h"
#include "core/strlcpy.h"
#include "db/press.h"
#include <assert.h>
#include <uv.h>

static work_end_fn_t *on_end = NULL;
static enum state state = INIT;
static atomic_bool cancel = false;
static struct uv_work_s work_request = {0};

static char hmm[FILENAME_SIZE] = {0};
static char db[FILENAME_SIZE] = {0};
static struct db_press db_press = {0};

static struct progress progress = {0};

void work_init(work_end_fn_t *on_end_fn)
{
    on_end = on_end_fn;
    state = INIT;
    cancel = false;
    progress_init(&progress, 0);
}

enum state work_state(void) { return state; }

int work_progress(void) { return progress_percent(&progress); }

static void run(struct uv_work_s *);
static void end(struct uv_work_s *, int status);

int work_run(char const *hmmfile, char const *dbfile)
{
    if (state == RUN) return einval("work thread is busy");
    assert(!cancel);

    if (strlcpy(hmm, hmmfile, sizeof(hmm)) >= sizeof(hmm))
        return enomem("file name is too long");

    if (strlcpy(db, dbfile, sizeof(db)) >= sizeof(db))
        return enomem("file name is too long");

    if (uv_queue_work(global_loop(), &work_request, &run, &end))
        return efail("failed to queue work");

    progress_init(&progress, 0);
    state = RUN;
    return RC_OK;
}

char const *work_hmmfile(void) { return hmm; }

void work_cancel(void)
{
    if (state != RUN) return;
    uv_cancel((struct uv_req_s *)&work_request);
    atomic_release(&cancel, true);
}

static void run(struct uv_work_s *w)
{
    (void)w;
    int rc = db_press_init(&db_press, hmm, db);
    if (rc)
    {
        state = FAIL;
        return;
    }

    progress_init(&progress, db_press_nsteps(&db_press));
    info("found %ld profiles. Pressing...", db_press_nsteps(&db_press));
    while (!(rc = db_press_step(&db_press)))
    {
        if (atomic_consume(&cancel))
        {
            info("cancelled");
            state = CANCEL;
            db_press_cleanup(&db_press, false);
            return;
        }

        progress_consume(&progress, 1);
    }

    if (rc != RC_END)
    {
        state = FAIL;
        db_press_cleanup(&db_press, false);
        return;
    }

    if ((rc = db_press_cleanup(&db_press, true)))
    {
        state = FAIL;
        return;
    }

    state = DONE;
    info("pressing has finished");
}

static void end(struct uv_work_s *w, int status)
{
    (void)w;
    if (status == UV_ECANCELED && state == RUN) state = CANCEL;
    atomic_release(&cancel, false);
    assert(state == CANCEL || state == DONE || state == FAIL);

    enum work_end_reason reason = 0;
    if (state == CANCEL) reason = CANCELLED;
    if (state == DONE) reason = SUCCESS;
    if (state == FAIL) reason = FAILURE;

    if (on_end) (*on_end)(reason, hmm);
}

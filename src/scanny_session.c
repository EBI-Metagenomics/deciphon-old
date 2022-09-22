#include "scanny_session.h"
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

    unsigned num_threads;
    struct scan_thread thread[NUM_THREADS];
    double lrt_threshold;

    atomic_bool cancel;
    enum state state;
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

void scanny_session_init(struct uv_loop_s *loop)
{
    session.loop = loop;
    session.cancel = false;
    atomic_store(&session.cancel, false);
    session.state = IDLE;
    progress_init(&session.progress, 0);
}

bool scanny_session_is_running(void) { return session.state == RUN; }

bool scanny_session_is_done(void) { return session.state == DONE; }

bool scanny_session_start(char const *seqs, char const *db, bool multi_hits,
                          bool hmmer3_compat)
{
    if (zc_strlcpy(session.seqs, seqs, PATH_SIZE) >= PATH_SIZE)
    {
        enomem("file path is too long");
        return false;
    }

    if (zc_strlcpy(session.db, db, PATH_SIZE) >= PATH_SIZE)
    {
        enomem("file path is too long");
        return false;
    }

    session.state = RUN;
    atomic_store(&session.cancel, false);
    work(0);
    // uv_queue_work(session.loop, &session.request, work, after_work);

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
    info("Preparing to scan...");

    JR_INIT(session.jr);
    char *json = (char *)xfile_readall(session.seqs);
    if (!json) goto cleanup_fail;

    if (jr_parse(session.jr, strlen(json), json))
    {
        eparse("failed to parse seqs json");
        goto cleanup_fail;
    }

    if (jr_type(session.jr) != JR_ARRAY)
    {
        eparse("failed to parse seqs json");
        goto cleanup_fail;
    }

    unsigned nseqs = (unsigned)jr_nchild(session.jr);
    assert(nseqs > 0);
    enum rc rc = scan_init(session.db, nseqs, 2, 0.01);

    jr_next(session.jr);

#if 0
    long seq_id = jr_long_of(session.jr, "id");
    long scan_id = jr_long_of(session.jr, "scan_id");
    char const *name = jr_string_of(session.jr, "name");
    char const *seq_data = jr_string_of(session.jr, "data");
    if (jr_error())
    {
        eparse("failed to parse seqs json");
        goto cleanup_fail;
    }

    FILE *fp = fopen(session.db, "rb");
    if (!fp)
    {
        eio("failed to open database");
        goto cleanup_fail;
    }
#endif

    free(json);
    return;

cleanup_fail:
    session.state = FAIL;
    free(json);
}

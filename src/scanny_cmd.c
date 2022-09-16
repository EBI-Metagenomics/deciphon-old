#include "scanny_cmd.h"
#include "core/c23.h"
#include "core/logging.h"
#include "core/xfile.h"
#include "jx.h"
#include "uv.h"
#include "zc.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

enum state
{
    SCAN_IDLE,
    SCAN_RUN,
    SCAN_DONE,
    SCAN_FAIL,
    SCAN_CANCEL,
};

static atomic_bool cancel_session = false;
static atomic_int session_progress = 0;
static enum state state = SCAN_IDLE;
static char const *state_string[] = {[SCAN_IDLE] = "SCAN_IDLE",
                                     [SCAN_RUN] = "SCAN_RUN",
                                     [SCAN_DONE] = "SCAN_DONE",
                                     [SCAN_FAIL] = "SCAN_FAIL",
                                     [SCAN_CANCEL] = "SCAN_CANCEL"};

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static char db_filepath[PATH_MAX] = {0};
static char seqs_filepath[PATH_MAX] = {0};
static struct uv_work_s request = {0};
static struct uv_loop_s *loop = nullptr;

static cmd_fn_t *scanny_cmds[] = {
#define X(_, A) &A,
    SCANNY_CMD_MAP(X)
#undef X
};

static inline char const *say_ok(void) { return "OK"; }

static inline char const *say_done(void) { return "DONE"; }

static inline char const *say_fail(void) { return "FAIL"; }

static inline char const *say_busy(void) { return "BUSY"; }

#define error_parse() error("failed to parse command")

static enum scanny_cmd parse(char const *);

cmd_fn_t *scanny_cmd(char const *cmd, struct uv_loop_s *loop_)
{
    loop = loop_;
    return scanny_cmds[parse(cmd)];
}

static void scan_session(struct uv_work_s *);
static void scan_cleanup(struct uv_work_s *, int status);

char const *scanny_cmd_invalid(struct cmd const *cmd) {}

char const *scanny_cmd_scan(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "sss"))
    {
        error_parse();
        return say_fail();
    }

    if (state == SCAN_RUN) return say_busy();
    atomic_store(&session_progress, 0);
    state = SCAN_RUN;

    char const *seqs = cmd->argv[1];
    char const *db = cmd->argv[2];

    if (zc_strlcpy(db_filepath, db, sizeof db_filepath) >= sizeof db_filepath)
    {
        enomem("file path is too long");
        return say_fail();
    }

    if (zc_strlcpy(seqs_filepath, seqs, sizeof seqs_filepath) >=
        sizeof seqs_filepath)
    {
        enomem("file path is too long");
        return say_fail();
    }

    atomic_store(&cancel_session, false);
    scan_session(0);
    // uv_queue_work(loop, &request, scan_session, scan_cleanup);

    return say_ok();
}

char const *scanny_cmd_cancel(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "s"))
    {
        error_parse();
        return say_fail();
    }

    if (state != SCAN_RUN) return say_done();

    if (atomic_load(&cancel_session))
    {
        int rc = uv_cancel((struct uv_req_s *)&request);
        if (rc)
        {
            warn(uv_strerror(rc));
            return say_fail();
        }
        return say_done();
    }
    atomic_store(&cancel_session, true);

    return say_ok();
}

char const *scanny_cmd_state(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "s"))
    {
        error_parse();
        return say_fail();
    }

    return state_string[state];
}

char const *scanny_cmd_progress(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "s"))
    {
        error_parse();
        return say_fail();
    }

    static char progress[5] = "100%";
    if (state == SCAN_RUN)
    {
        sprintf(progress, "%d%%", atomic_load(&session_progress));
        return progress;
    }
    else if (state == SCAN_DONE)
        return "100%";
    return say_fail();
}

static JR_DECLARE(jr, 128);

static void scan_session(struct uv_work_s *req)
{
    (void)req;
    JR_INIT(jr);
    char *json = (char *)xfile_readall(seqs_filepath);
    if (!json)
    {
        state = SCAN_FAIL;
        free(json);
        return;
    }

    if (jr_parse(jr, strlen(json), json))
    {
        eparse("failed to parse seqs json");
        state = SCAN_FAIL;
        free(json);
        return;
    }
    free(json);

    if (jr_type(jr) != JR_ARRAY)
    {
        eparse("failed to parse seqs json");
        state = SCAN_FAIL;
        free(json);
        return;
    }

    jr_next(jr);

    long id = jr_long_of(jr, "id");
    long scan_id = jr_long_of(jr, "scan_id");
    char const *name = jr_string_of(jr, "name");
    char const *seq = jr_string_of(jr, "data");
    if (jr_error())
    {
        eparse("failed to parse seqs json");
        state = SCAN_FAIL;
        free(json);
        return;
    }
    printf("%ld %ld\n %s\n %s\n", id, scan_id, name, seq);
}

static void scan_cleanup(struct uv_work_s *req, int status)
{
    (void)req;
    if (status == UV_ECANCELED) state = SCAN_CANCEL;
    atomic_store(&cancel_session, false);
}

static enum scanny_cmd parse(char const *cmd)
{
#define X(A, _)                                                                \
    if (!strcmp(cmd, STRINGIFY(A))) return SCANNY_CMD_##A;
    SCANNY_CMD_MAP(X)
#undef X
    return SCANNY_CMD_INVALID;
}

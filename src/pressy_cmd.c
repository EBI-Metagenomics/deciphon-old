#include "pressy_cmd.h"
#include "core/c23.h"
#include "core/logging.h"
#include "core/pp.h"
#include "core/xfile.h"
#include "db/press.h"
#include "uv.h"
#include "zc.h"
#include <stdatomic.h>
#include <string.h>

enum state
{
    PRESS_IDLE,
    PRESS_RUN,
    PRESS_DONE,
    PRESS_FAIL,
    PRESS_CANCEL,
};

static atomic_bool cancel_session = false;
static enum state state = PRESS_IDLE;
static struct db_press db_press = {0};
static char const *state_string[] = {[PRESS_IDLE] = "PRESS_IDLE",
                                     [PRESS_RUN] = "PRESS_RUN",
                                     [PRESS_DONE] = "PRESS_DONE",
                                     [PRESS_FAIL] = "PRESS_FAIL",
                                     [PRESS_CANCEL] = "PRESS_CANCEL"};

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static char hmm_filepath[PATH_MAX] = {0};
static char db_filepath[PATH_MAX] = {0};
static struct uv_work_s request = {0};
static struct uv_loop_s *loop = nullptr;

struct baton
{
    enum state state;
    char hmm_filepath[PATH_MAX];
    char db_filepath[PATH_MAX];
};

static cmd_fn_t *pressy_cmds[] = {
#define X(_, A) &A,
    PRESSY_CMD_MAP(X)
#undef X
};

static inline char const *say_ok(void) { return "OK"; }

static inline char const *say_done(void) { return "DONE"; }

static inline char const *say_fail(void) { return "FAIL"; }

static inline char const *say_busy(void) { return "BUSY"; }

#define error_parse() error("failed to parse command")

static enum pressy_cmd parse(char const *);

cmd_fn_t *pressy_cmd(char const *cmd, struct uv_loop_s *loop_)
{
    loop = loop_;
    return pressy_cmds[parse(cmd)];
}

static void press_session(struct uv_work_s *);
static void press_cleanup(struct uv_work_s *, int status);

char const *pressy_cmd_invalid(struct cmd const *cmd) {}

char const *pressy_cmd_press(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "ss"))
    {
        error_parse();
        return say_fail();
    }

    if (state == PRESS_RUN) return say_busy();
    state = PRESS_RUN;

    char const *p = cmd->argv[1];
    if (zc_strlcpy(hmm_filepath, p, sizeof hmm_filepath) >= sizeof hmm_filepath)
    {
        enomem("file path is too long");
        return say_fail();
    }

    strcpy(db_filepath, hmm_filepath);
    db_filepath[strlen(db_filepath) - 3] = 'd';
    db_filepath[strlen(db_filepath) - 2] = 'c';
    db_filepath[strlen(db_filepath) - 1] = 'p';

    atomic_store(&cancel_session, false);
    uv_queue_work(loop, &request, press_session, press_cleanup);

    return say_ok();
}

char const *pressy_cmd_cancel(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "s"))
    {
        error_parse();
        return say_fail();
    }

    if (state != PRESS_RUN) return say_done();

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

char const *pressy_cmd_state(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "s"))
    {
        error_parse();
        return say_fail();
    }

    return state_string[state];
}

static void press_session(struct uv_work_s *req)
{
    (void)req;
    if (!xfile_is_readable(hmm_filepath))
    {
        eio("hmm file is not readable");
        state = PRESS_FAIL;
        return;
    }

    FILE *hmm = fopen(hmm_filepath, "rb");
    if (!hmm)
    {
        eio("failed to open hmm file");
        state = PRESS_FAIL;
        return;
    }

    FILE *db = fopen(db_filepath, "wb");
    if (!db)
    {
        eio("failed to open db file");
        state = PRESS_FAIL;
        fclose(hmm);
        return;
    }

    enum rc rc = db_press_init(&db_press, hmm, db);
    if (rc)
    {
        state = PRESS_FAIL;
        fclose(hmm);
        fclose(db);
        return;
    }

    // unsigned nsteps = db_press_nsteps(&db_press);

    while (!(rc = db_press_step(&db_press)))
    {
        if (atomic_load(&cancel_session))
        {
            state = PRESS_CANCEL;
            fclose(hmm);
            fclose(db);
            return;
        }
    }

    if (rc != RC_END)
    {
        state = PRESS_FAIL;
        fclose(hmm);
        fclose(db);
        return;
    }

    rc = db_press_cleanup(&db_press);
    if (rc)
    {
        state = PRESS_FAIL;
        fclose(hmm);
        fclose(db);
        return;
    }
    state = PRESS_DONE;
}

static void press_cleanup(struct uv_work_s *req, int status)
{
    if (status == UV_ECANCELED) state = PRESS_CANCEL;
    atomic_store(&cancel_session, false);
}

static enum pressy_cmd parse(char const *cmd)
{
#define X(A, _)                                                                \
    if (!strcmp(cmd, STRINGIFY(A))) return PRESSY_CMD_##A;
    PRESSY_CMD_MAP(X)
#undef X
    return PRESSY_CMD_INVALID;
}

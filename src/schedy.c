#include "deciphon/core/compiler.h"
#include "deciphon/core/getcmd.h"
#include "deciphon/core/liner.h"
#include "deciphon/core/logging.h"
#include "deciphon/core/looper.h"
#include "deciphon/core/to.h"
#include "deciphon/sched/api.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>

static void ioerror_cb(void);
static void newline_cb(char *line);
static void onterm_cb(void);

static struct looper looper = {0};
static struct liner liner = {0};
static struct sched_hmm hmm = {0};

#define CMD_MAP(X)                                                             \
    X(INVALID)                                                                 \
    X(INIT)                                                                    \
    X(IS_REACHABLE)                                                            \
    X(WIPE)                                                                    \
    X(UPLOAD_HMM)                                                              \
    X(GET_HMM)                                                                 \
    X(GET_HMM_BY_JOB_ID)                                                       \
    X(DOWNLOAD_HMM)                                                            \
    X(API_UPLOAD_DB)                                                           \
    X(GET_DB)                                                                  \
    X(NEXT_PEND_JOB)                                                           \
    X(SET_JOB_STATE)                                                           \
    X(DOWNLOAD_DB)                                                             \
    X(UPLOAD_PRODS_FILE)                                                       \
    X(SCAN_NEXT_SEQ)                                                           \
    X(SCAN_NUM_SEQS)                                                           \
    X(GET_SCAN_BY_JOB_ID)                                                      \
    X(INCREMENT_JOB_PROGRESS)

enum cmd
{
#define X(A) CMD_##A,
    CMD_MAP(X)
#undef X
};

enum cmd parse_command(char const *cmd)
{
#define X(A)                                                                   \
    if (!strcmp(cmd, STRINGIFY(A))) return CMD_##A;
    CMD_MAP(X)
#undef X
    return CMD_INVALID;
}

struct split
{
    char *command;
    char *payload;
};

static struct split split_line(char *line);

enum
{
    PAYLOAD_ARGC_MAX = 4,
};

struct payload_args
{
    unsigned argc;
    char const *argv[PAYLOAD_ARGC_MAX];
};

int main(void)
{
    looper_init(&looper, onterm_cb);
    liner_init(&liner, &looper, ioerror_cb, newline_cb);
    liner_open(&liner, 0);

    looper_run(&looper);

    looper_cleanup(&looper);
}

static void ioerror_cb(void)
{
    error("io error");
    looper_terminate(&looper);
}

static inline void say_ok(void) { puts("OK"); }

static inline void say_fail(void) { puts("FAIL"); }

static inline void say_yes(void) { puts("YES"); }

static inline void say_no(void) { puts("NO"); }

static void exec_cmd(struct getcmd const *gc)
{
    enum rc rc = RC_OK;
    enum cmd cmd = parse_command(gc->argv[0]);

    if (cmd == CMD_INVALID)
    {
        warn("invalid command");
        say_fail();
    }
    if (cmd == CMD_INIT)
    {
        if ((rc = api_init(gc->argv[1], gc->argv[2])))
        {
            error(RC_STRING(rc));
            say_fail();
            looper_terminate(&looper);
        }
        else
            say_ok();
    }
    if (cmd == CMD_IS_REACHABLE)
    {
        if (api_is_reachable())
            say_yes();
        else
            say_no();
    }
    if (cmd == CMD_WIPE)
    {
        if ((rc = api_wipe()))
        {
            error(RC_STRING(rc));
            say_fail();
        }
        else
            say_ok();
    }
    if (cmd == CMD_UPLOAD_HMM)
    {
        if ((rc = api_upload_hmm(gc->argv[1], &hmm)))
        {
            error(RC_STRING(rc));
            say_fail();
        }
        say_ok();
    }
    if (cmd == CMD_GET_HMM)
    {
        if (!getcmd_check(gc, "si"))
        {
            error("failed to parse command");
            say_fail();
        }
        else
        {
            if ((rc = api_get_hmm(getcmd_i64(gc, 1), &hmm)))
            {
                error(RC_STRING(rc));
                say_fail();
            }
            else
                say_ok();
        }
    }
}

static void newline_cb(char *line)
{
    struct getcmd getcmd = {0};
    if (!getcmd_parse(&getcmd, line)) error("too many arguments");
    exec_cmd(&getcmd);
}

static void onterm_cb(void) { liner_close(&liner); }

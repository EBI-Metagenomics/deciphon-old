#include "deciphon/core/compiler.h"
#include "deciphon/core/liner.h"
#include "deciphon/core/logging.h"
#include "deciphon/core/looper.h"
#include "deciphon/sched/api.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>

static void ioerror_cb(void);
static void parse_newline(char *line);
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

static void payload_parse(struct payload_args *args, char *payload)
{
    char *token = strtok(payload, "&");
    args->argc = 0;
    while (token && args->argc < PAYLOAD_ARGC_MAX)
    {
        args->argv[args->argc++] = token;
        token = strtok(0, "&");
    }

    while (args->argc < PAYLOAD_ARGC_MAX)
        args->argv[args->argc++] = "";
}

int main(void)
{
    looper_init(&looper, onterm_cb);
    liner_init(&liner, &looper, ioerror_cb, parse_newline);
    liner_open(&liner, 0);

    looper_run(&looper);

    looper_cleanup(&looper);
}

static void ioerror_cb(void)
{
    error("io error");
    looper_terminate(&looper);
}

static inline say_ok(void) { puts("OK"); }

static inline say_fail(void) { puts("FAIL"); }

static inline say_yes(void) { puts("YES"); }

static inline say_no(void) { puts("NO"); }

static void exec_cmd(enum cmd cmd, struct payload_args *args)
{
    enum rc rc = RC_OK;

    if (cmd == CMD_INVALID)
    {
        warn("invalid command");
        say_fail();
    }
    if (cmd == CMD_INIT)
    {
        if ((rc = api_init(args->argv[0], args->argv[1])))
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
        if ((rc = api_upload_hmm(args->argv[0], &hmm)))
        {
            error(RC_STRING(rc));
            say_fail();
        }
        say_ok();
    }
}

static void parse_newline(char *line)
{
    struct split split = split_line(line);
    enum cmd cmd = parse_command(split.command);
    struct payload_args args = {0};
    payload_parse(&args, split.payload);
    exec_cmd(cmd, &args);
}

static void onterm_cb(void) { liner_close(&liner); }

static struct split split_line(char *line)
{
    char *command = strtok(line, " ");
    if (!command) return (struct split){"", ""};

    char *payload = strtok(0, "\n");
    if (!payload) return (struct split){command, ""};

    return (struct split){command, payload};
}

#include "deciphon/core/compiler.h"
#include "deciphon/core/getcmd.h"
#include "deciphon/core/liner.h"
#include "deciphon/core/logging.h"
#include "deciphon/core/looper.h"
#include "deciphon/core/to.h"
#include "deciphon/sched/api.h"
#include "schedy_cmd.h"
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

static schedy_cmd_fn_t *schedy_cmds[] = {
    &schedy_cmd_invalid, &schedy_cmd_init,       &schedy_cmd_is_reachable,
    &schedy_cmd_wipe,    &schedy_cmd_upload_hmm, &schedy_cmd_get_hmm,
};

static void exec_cmd(struct getcmd const *gc)
{
    enum rc rc = RC_OK;
    enum cmd cmd = parse_command(gc->argv[0]);
    (*schedy_cmds[cmd])(gc);

#if 0
    if (cmd == CMD_DOWNLOAD_HMM)
    {
        if (!getcmd_check(gc, "ss"))
        {
            error("failed to parse command");
            say_fail();
        }
        else
        {
            if ((rc = api_download_hmm(getcmd_i64(gc, 1), fp)))
            {
                error(RC_STRING(rc));
                say_fail();
            }
            else
                say_ok();
        }
    }
#endif
    // enum rc api_get_hmm_by_job_id(int64_t job_id, struct sched_hmm *);
    // enum rc api_download_hmm(int64_t id, FILE *fp);
}

static void newline_cb(char *line)
{
    struct getcmd getcmd = {0};
    if (!getcmd_parse(&getcmd, line)) error("too many arguments");
    exec_cmd(&getcmd);
}

static void onterm_cb(void) { liner_close(&liner); }

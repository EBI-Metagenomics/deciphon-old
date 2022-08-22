#include "deciphon/core/compiler.h"
#include "deciphon/core/getcmd.h"
#include "deciphon/core/logging.h"
#include "deciphon/core/to.h"
#include "deciphon/loop/liner.h"
#include "deciphon/loop/looper.h"
#include "deciphon/sched/api.h"
#include "schedy_cmd.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>

static void ioerror_cb(void);
static void newline_cb(char *line);
static void onterm_cb(void);

static struct looper looper = {0};
static struct liner *liner = 0;

#define CMD_MAP(X)                                                             \
    X(INVALID, schedy_cmd_invalid)                                             \
    X(CONNECT, schedy_cmd_connect)                                             \
    X(ONLINE, schedy_cmd_online)                                               \
    X(WIPE, schedy_cmd_wipe)                                                   \
                                                                               \
    X(HMM_UP, schedy_cmd_hmm_up)                                               \
    X(HMM_DL, schedy_cmd_hmm_dl)                                               \
    X(HMM_GET_BY_ID, schedy_cmd_hmm_get_by_id)                                 \
    X(HMM_GET_BY_XXH3, schedy_cmd_hmm_get_by_xxh3)                             \
    X(HMM_GET_BY_JOB_ID, schedy_cmd_hmm_get_by_job_id)                         \
    X(HMM_GET_BY_FILENAME, schedy_cmd_hmm_get_by_filename)                     \
                                                                               \
    X(DB_UP, schedy_cmd_db_up)                                                 \
    X(DB_DL, schedy_cmd_db_dl)                                                 \
    X(DB_GET_BY_ID, schedy_cmd_db_get_by_id)                                   \
    X(DB_GET_BY_XXH3, schedy_cmd_db_get_by_xxh3)                               \
    X(DB_GET_BY_JOB_ID, schedy_cmd_db_get_by_job_id)                           \
    X(DB_GET_BY_FILENAME, schedy_cmd_db_get_by_filename)                       \
                                                                               \
    X(JOB_NEXT_PEND, schedy_cmd_job_next_pend)                                 \
    X(JOB_SET_STATE, schedy_cmd_job_set_state)                                 \
    X(JOB_INC_PROGRESS, schedy_cmd_job_inc_progress)                           \
                                                                               \
    X(PRODS_FILE_UP, schedy_cmd_prods_file_up)                                 \
                                                                               \
    X(SCAN_SCAN_NEXT_SEQ, schedy_cmd_scan_next_seq)                            \
    X(SCAN_SCAN_NUM_SEQS, schedy_cmd_scan_num_seqs)                            \
    X(SCAN_SCAN_GET_BY_JOB_ID, schedy_cmd_scan_get_by_job_id)

enum cmd
{
#define X(A, _) CMD_##A,
    CMD_MAP(X)
#undef X
};

enum cmd parse_command(char const *cmd)
{
#define X(A, _)                                                                \
    if (!strcmp(cmd, STRINGIFY(A))) return CMD_##A;
    CMD_MAP(X)
#undef X
    return CMD_INVALID;
}

int main(void)
{
    looper_init(&looper, onterm_cb);
    if (!(liner = liner_new(&looper, ioerror_cb, newline_cb))) return 1;
    liner_open(liner, 0);

    looper_run(&looper);

    looper_cleanup(&looper);
}

static void ioerror_cb(void)
{
    error("io error");
    looper_terminate(&looper);
}

static schedy_cmd_fn_t *schedy_cmds[] = {
#define X(_, A) &A,
    CMD_MAP(X)
#undef X
};

static void exec_cmd(struct getcmd const *gc)
{
    enum cmd cmd = parse_command(gc->argv[0]);
    char const *msg = (*schedy_cmds[cmd])(gc);
    puts(msg);
    fflush(stdout);
}

static void newline_cb(char *line)
{
    struct getcmd getcmd = {0};
    if (!getcmd_parse(&getcmd, line)) error("too many arguments");
    exec_cmd(&getcmd);
}

static void onterm_cb(void) { liner_del(liner); }

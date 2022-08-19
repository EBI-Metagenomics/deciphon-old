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

#define CMD_MAP(X)                                                             \
    X(INVALID, schedy_cmd_invalid)                                             \
    X(INIT, schedy_cmd_init)                                                   \
    X(IS_REACHABLE, schedy_cmd_is_reachable)                                   \
    X(WIPE, schedy_cmd_wipe)                                                   \
    X(UPLOAD_HMM, schedy_cmd_upload_hmm)                                       \
    X(GET_HMM_BY_ID, schedy_cmd_get_hmm_by_id)                                 \
    X(GET_HMM_BY_XXH3, schedy_cmd_get_hmm_by_xxh3)                             \
    X(GET_HMM_BY_JOB_ID, schedy_cmd_get_hmm_by_job_id)                         \
    X(GET_HMM_BY_FILENAME, schedy_cmd_get_hmm_by_filename)                     \
    X(DOWNLOAD_HMM, schedy_cmd_download_hmm)                                   \
    X(API_UPLOAD_DB, schedy_cmd_api_upload_db)                                 \
    X(GET_DB, schedy_cmd_get_db)                                               \
    X(NEXT_PEND_JOB, schedy_cmd_next_pend_job)                                 \
    X(SET_JOB_STATE, schedy_cmd_set_job_state)                                 \
    X(DOWNLOAD_DB, schedy_cmd_download_db)                                     \
    X(UPLOAD_PRODS_FILE, schedy_cmd_upload_prods_file)                         \
    X(SCAN_NEXT_SEQ, schedy_cmd_scan_next_seq)                                 \
    X(SCAN_NUM_SEQS, schedy_cmd_scan_num_seqs)                                 \
    X(GET_SCAN_BY_JOB_ID, schedy_cmd_get_scan_by_job_id)                       \
    X(INCREMENT_JOB_PROGRESS, schedy_cmd_increment_job_progress)

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
#define X(_, A) &A,
    CMD_MAP(X)
#undef X
};

static void exec_cmd(struct getcmd const *gc)
{
    enum cmd cmd = parse_command(gc->argv[0]);
    (*schedy_cmds[cmd])(gc);
}

static void newline_cb(char *line)
{
    struct getcmd getcmd = {0};
    if (!getcmd_parse(&getcmd, line)) error("too many arguments");
    exec_cmd(&getcmd);
}

static void onterm_cb(void) { liner_close(&liner); }

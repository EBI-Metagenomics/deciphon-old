#include "argless.h"
#include "core/api.h"
#include "core/compiler.h"
#include "core/getcmd.h"
#include "core/io.h"
#include "core/logging.h"
#include "core/to.h"
#include "loop/liner.h"
#include "loop/looper.h"
#include "loop/writer.h"
#include "schedy_cmd.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void ioerror_cb(void);
static void newline_cb(char *line);
static void onterm_cb(void);

static struct looper looper = {0};
static struct liner liner = {0};
static struct writer *writer = 0;
static IO_DECLARE(io);

static struct argl_option const options[] = {
    {"input", 'i', "INPUT", "Input stream. Defaults to `STDIN'.", false},
    {"output", 'o', "OUTPUT", "Output stream. Defaults to `STDOUT'.", false},
    ARGL_DEFAULT_OPTS,
    ARGL_NULL_OPT,
};

static struct argl argl = {.options = options,
                           .args_doc = nullptr,
                           .doc = "Schedy program.",
                           .version = "1.0.0"};

static inline char const *get(char const *name, char const *default_value)
{
    return argl_has(&argl, name) ? argl_get(&argl, name) : default_value;
}

int main(int argc, char *argv[])
{
    argl_parse(&argl, argc, argv);
    if (argl_nargs(&argl)) argl_usage(&argl);

    looper_init(&looper, onterm_cb);
    io_init(&io, looper.loop);

    if (!io_setup(&io, get("input", nullptr), get("output", nullptr)))
        return EXIT_FAILURE;

    liner_init(&liner, &looper, ioerror_cb, newline_cb);
    if (!(writer = writer_new(&looper, io.output.fd))) return EXIT_FAILURE;
    liner_open(&liner, io.input.fd);

    looper_run(&looper);

    looper_cleanup(&looper);

    io_cleanup(&io);
    return EXIT_SUCCESS;
}

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
    X(DB_GET_BY_HMM_ID, schedy_cmd_db_get_by_hmm_id)                           \
    X(DB_GET_BY_FILENAME, schedy_cmd_db_get_by_filename)                       \
                                                                               \
    X(JOB_NEXT_PEND, schedy_cmd_job_next_pend)                                 \
    X(JOB_SET_STATE, schedy_cmd_job_set_state)                                 \
    X(JOB_INC_PROGRESS, schedy_cmd_job_inc_progress)                           \
                                                                               \
    X(SCAN_DL_SEQS, schedy_cmd_scan_dl_seqs)                                   \
    X(SCAN_SEQ_COUNT, schedy_cmd_scan_seq_count)                               \
    X(SCAN_SUBMIT, schedy_cmd_scan_submit)                                     \
    X(SCAN_NEXT_SEQ, schedy_cmd_scan_next_seq)                                 \
    X(SCAN_GET_BY_JOB_ID, schedy_cmd_scan_get_by_job_id)                       \
                                                                               \
    X(PRODS_FILE_UP, schedy_cmd_prods_file_up)

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
    writer_put(writer, msg);
}

static void newline_cb(char *line)
{
    static struct getcmd getcmd = {0};
    if (!getcmd_parse(&getcmd, line)) error("too many arguments");
    exec_cmd(&getcmd);
}

static void onterm_cb(void)
{
    liner_close(&liner);
    if (writer) writer_del(writer);
}

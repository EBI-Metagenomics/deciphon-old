#include "argless.h"
#include "deciphon/api.h"
#include "deciphon/core/compiler.h"
#include "deciphon/core/getcmd.h"
#include "deciphon/core/logging.h"
#include "deciphon/core/to.h"
#include "deciphon/loop/liner.h"
#include "deciphon/loop/looper.h"
#include "deciphon/loop/writer.h"
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

static struct
{
    struct
    {
        int fd;
        struct uv_fs_s req;
    } input;
    struct
    {
        int fd;
        struct uv_fs_s req;
    } output;
} stream = {.input = {STDIN_FILENO, {0}}, .output = {STDOUT_FILENO, {0}}};

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

static bool stream_setup(void);
static void stream_cleanup(void);

int main(int argc, char *argv[])
{
    argl_parse(&argl, argc, argv);
    if (argl_nargs(&argl)) argl_usage(&argl);

    if (!stream_setup()) return EXIT_FAILURE;

    looper_init(&looper, onterm_cb);
    liner_init(&liner, &looper, ioerror_cb, newline_cb);
    if (!(writer = writer_new(&looper, stream.output.fd))) return EXIT_FAILURE;
    liner_open(&liner, stream.input.fd);

    looper_run(&looper);

    looper_cleanup(&looper);

    stream_cleanup();
    return EXIT_SUCCESS;
}

static bool stream_setup(void)
{
    if (argl_has(&argl, "input"))
    {
        int fd = uv_fs_open(looper.loop, &stream.input.req,
                            argl_get(&argl, "input"), O_RDONLY, 0, NULL);
        if (fd == -1)
        {
            eio("failed to open input for reading");
            goto cleanup;
        }
        stream.input.fd = fd;
    }

    if (argl_has(&argl, "output"))
    {
        int fd = uv_fs_open(looper.loop, &stream.output.req,
                            argl_get(&argl, "output"), O_WRONLY, 0, NULL);
        if (fd == -1)
        {
            eio("failed to open output for writing");
            goto cleanup;
        }
        stream.output.fd = fd;
    }
    return true;

cleanup:
    stream_cleanup();
    return false;
}

static void stream_cleanup(void)
{
    if (stream.input.fd != STDIN_FILENO)
        uv_fs_close(looper.loop, &stream.input.req, stream.input.fd, NULL);
    if (stream.output.fd != STDOUT_FILENO)
        uv_fs_close(looper.loop, &stream.output.req, stream.output.fd, NULL);

    stream.input.fd = STDIN_FILENO;
    stream.output.fd = STDOUT_FILENO;
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

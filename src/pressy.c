#include "argless.h"
#include "core/logging.h"
#include "loop/liner.h"
#include "loop/looper.h"
#include "loop/writer.h"
#include <stdlib.h>
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
                           .doc = "pressy program.",
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

static void ioerror_cb(void)
{
    error("io error");
    looper_terminate(&looper);
}

static void newline_cb(char *line) { press_line(); }

static void onterm_cb(void)
{
    liner_close(&liner);
    if (writer) writer_del(writer);
}

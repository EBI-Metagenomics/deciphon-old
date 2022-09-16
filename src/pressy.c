#include "argless.h"
#include "core/cmd.h"
#include "core/logging.h"
#include "hmr/hmr.h"
#include "loop/io.h"
#include "loop/liner.h"
#include "loop/looper.h"
#include "loop/writer.h"
#include "model/protein_h3reader.h"
#include "pressy_cmd.h"
#include <stdlib.h>
#include <unistd.h>

static struct looper looper = {0};
static struct liner liner = {0};
static struct writer writer = {0};
static struct io input = {0};
static struct io output = {0};

static struct argl_option const options[] = {
    {"input", 'i', "INPUT", "Input stream. Defaults to `STDIN'.", false},
    {"output", 'o', "OUTPUT", "Output stream. Defaults to `STDOUT'.", false},
    ARGL_DEFAULT_OPTS,
    ARGL_NULL_OPT,
};

static struct argl argl = {.options = options,
                           .args_doc = nullptr,
                           .doc = "Pressy program.",
                           .version = "1.0.0"};

static inline char const *get(char const *name, char const *default_value)
{
    return argl_has(&argl, name) ? argl_get(&argl, name) : default_value;
}

static void looper_onterm_cb(void);
static void input_onopen_cb(bool ok);
static void input_onclose_cb(void);
static void output_onopen_cb(bool ok);
static void output_onclose_cb(void);
static void liner_ioerror_cb(void);
static void liner_onread_cb(char *line);
static void liner_onclose_cb(void);
static void writer_onclose_cb(void);

int main(int argc, char *argv[])
{
    argl_parse(&argl, argc, argv);
    if (argl_nargs(&argl)) argl_usage(&argl);

    if (setenv("UV_THREADPOOL_SIZE", "1", true))
        warn("failed to set UV_THREADPOOL_SIZE=1");

    looper_init(&looper, &looper_onterm_cb);

    liner_init(&liner, looper.loop, &liner_ioerror_cb, &liner_onread_cb,
               &liner_onclose_cb);
    writer_init(&writer, looper.loop, &writer_onclose_cb);

    io_init(&input, looper.loop, &input_onopen_cb, &input_onclose_cb);
    io_open(&input, get("input", "&1"), UV_FS_O_RDONLY, 0);

    io_init(&output, looper.loop, &output_onopen_cb, &output_onclose_cb);
    io_open(&output, get("output", "&2"), UV_FS_O_WRONLY, UV_FS_O_CREAT);

    looper_run(&looper);
    looper_cleanup(&looper);

    return EXIT_SUCCESS;
}

static void looper_onterm_cb(void)
{
    liner_close(&liner);
    writer_close(&writer);
    struct cmd cmd = {0};
    cmd_parse(&cmd, "CANCEL");
    pressy_cmd_cancel(&cmd);
}

static void input_onopen_cb(bool ok)
{
    if (!ok)
    {
        looper_terminate(&looper);
        return;
    }
    liner_open(&liner, io_fd(&input));
}

static void output_onopen_cb(bool ok)
{
    if (!ok)
    {
        looper_terminate(&looper);
        return;
    }
    writer_open(&writer, io_fd(&output));
}

static void input_onclose_cb(void) {}

static void output_onclose_cb(void) {}

static void liner_ioerror_cb(void)
{
    error("io error");
    looper_terminate(&looper);
}

static void liner_onread_cb(char *line)
{
    static struct cmd gc = {0};
    if (!cmd_parse(&gc, line)) error("too many arguments");
    writer_put(&writer, (*pressy_cmd(gc.argv[0], looper.loop))(&gc));
}

static void liner_onclose_cb(void) { io_close(&input); }

static void writer_onclose_cb(void) { io_close(&output); }

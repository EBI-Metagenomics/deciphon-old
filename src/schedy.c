#include "argless.h"
#include "core/api.h"
#include "core/compiler.h"
#include "core/logging.h"
#include "core/to.h"
#include "loop/io.h"
#include "loop/liner.h"
#include "loop/looper.h"
#include "loop/writer.h"
#include "schedy_cmd.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static struct looper looper = {0};
static struct liner liner = {0};
static struct writer writer = {0};
static struct io io = {0};

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

static void looper_onterm_cb(void);
static void io_onopen_cb(bool ok);
static void io_onclose_cb(void);
static void liner_ioerror_cb(void);
static void liner_onread_cb(char *line);
static void liner_onclose_cb(void);
static void writer_onclose_cb(void);

static bool liner_closed = false;
static bool writer_closed = false;

int main(int argc, char *argv[])
{
    argl_parse(&argl, argc, argv);
    if (argl_nargs(&argl)) argl_usage(&argl);

    looper_init(&looper, &looper_onterm_cb);

    liner_init(&liner, looper.loop, &liner_ioerror_cb, &liner_onread_cb,
               &liner_onclose_cb);
    writer_init(&writer, looper.loop, &writer_onclose_cb);

    io_init(&io, looper.loop, &io_onopen_cb, &io_onclose_cb);
    io_setup(&io, get("input", nullptr), get("output", nullptr));

    looper_run(&looper);
    looper_cleanup(&looper);

    return EXIT_SUCCESS;
}

static void looper_onterm_cb(void)
{
    liner_close(&liner);
    writer_close(&writer);
}

static void io_onopen_cb(bool ok)
{
    if (!ok)
    {
        looper_terminate(&looper);
        return;
    }

    liner_open(&liner, io.input.fd);
    writer_open(&writer, io.output.fd);
}

static void io_onclose_cb(void) {}

static void liner_ioerror_cb(void)
{
    error("io error");
    looper_terminate(&looper);
}

static void liner_onread_cb(char *line)
{
    static struct cmd gc = {0};
    if (!cmd_parse(&gc, line)) error("too many arguments");
    writer_put(&writer, (*schedy_cmd(gc.argv[0]))(&gc));
}

static void liner_onclose_cb(void)
{
    liner_closed = true;
    if (liner_closed && writer_closed) io_close(&io);
}

static void writer_onclose_cb(void)
{
    writer_closed = true;
    if (liner_closed && writer_closed) io_close(&io);
}

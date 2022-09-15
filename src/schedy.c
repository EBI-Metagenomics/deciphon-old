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

static void ioerror_cb(void);
static void newline_cb(char *line);
static void onterm_cb(void);

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

static void ioerror_cb(void)
{
    error("io error");
    looper_terminate(&looper);
}

static void newline_cb(char *line)
{
    static struct getcmd gc = {0};
    if (!getcmd_parse(&gc, line)) error("too many arguments");
    writer_put(writer, (*schedy_cmd(gc.argv[0]))(&gc));
}

static void onterm_cb(void)
{
    liner_close(&liner);
    if (writer) writer_del(writer);
}

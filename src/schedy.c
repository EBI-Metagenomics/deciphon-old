#include "argless.h"
#include "core/api.h"
#include "core/compiler.h"
#include "core/logging.h"
#include "core/to.h"
#include "loop/io.h"
#include "loop/looper.h"
#include "loop/reader.h"
#include "loop/writer.h"
#include "schedy_cmd.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static struct looper looper = {0};
static struct reader reader = {0};
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
                           .doc = "Schedy program.",
                           .version = "1.0.0"};

static inline char const *get(char const *name, char const *default_value)
{
    return argl_has(&argl, name) ? argl_get(&argl, name) : default_value;
}

static void looper_onterm(void);
static void input_onopen(bool ok);
static void input_onclose(void);
static void output_onopen(bool ok);
static void output_onclose(void);
static void reader_onerror(void);
static void reader_onread(char *line);
static void reader_onclose(void);
static void writer_onclose(void);

int main(int argc, char *argv[])
{
    argl_parse(&argl, argc, argv);
    if (argl_nargs(&argl)) argl_usage(&argl);

    looper_init(&looper, &looper_onterm);

    reader_init(&reader, looper.loop, &reader_onerror, &reader_onread,
                &reader_onclose);
    writer_init(&writer, looper.loop, &writer_onclose);

    io_init(&input, looper.loop, &input_onopen, &input_onclose);
    io_open(&input, get("input", "&1"), UV_FS_O_RDONLY, 0);

    io_init(&output, looper.loop, &output_onopen, &output_onclose);
    io_open(&output, get("output", "&2"), UV_FS_O_WRONLY, UV_FS_O_CREAT);

    looper_run(&looper);
    looper_cleanup(&looper);

    return EXIT_SUCCESS;
}

static void looper_onterm(void)
{
    reader_close(&reader);
    writer_close(&writer);
}

static void input_onopen(bool ok)
{
    if (!ok)
    {
        looper_terminate(&looper);
        return;
    }
    reader_open(&reader, io_fd(&input));
}

static void output_onopen(bool ok)
{
    if (!ok)
    {
        looper_terminate(&looper);
        return;
    }
    writer_open(&writer, io_fd(&output));
}

static void input_onclose(void) {}

static void output_onclose(void) {}

static void reader_onerror(void)
{
    error("io error");
    looper_terminate(&looper);
}

static void reader_onread(char *line)
{
    static struct cmd gc = {0};
    if (!cmd_parse(&gc, line)) error("too many arguments");
    writer_put(&writer, (*schedy_cmd(gc.argv[0]))(&gc));
}

static void reader_onclose(void) { io_close(&input); }

static void writer_onclose(void) { io_close(&output); }

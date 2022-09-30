#include "schedy/schedy.h"
#include "argless.h"
#include "core/daemonize.h"
#include "core/logy.h"
#include "lazylog.h"
#include "schedy/cmd.h"
#include <stdlib.h>

struct schedy schedy = {0};
static struct cmd cmd = {0};

static struct argl_option const options[] = {
    {"input", 'i', ARGL_TEXT("INPUT", "&1"), "Input stream."},
    {"output", 'o', ARGL_TEXT("OUTPUT", "&2"), "Output stream."},
    {"logstream", 's', ARGL_TEXT("LOGSTREAM", "&3"), "Logging stream."},
    {"loglevel", 'l', ARGL_TEXT("LOGLEVEL", "2"), "Logging level."},
    {"pid", 'p', ARGL_TEXT("PIDFILE", ARGL_NULL), "PID file."},
    {"daemon", 'D', ARGL_FLAG(), "Daemonize this program."},
    ARGL_DEFAULT,
    ARGL_END,
};

static struct argl argl = {.options = options,
                           .args_doc = nullptr,
                           .doc = "Schedy program.",
                           .version = "1.0.0"};

static void onlooper_term(void *);
static void oneof(void *);
static void onerror(void *);
static void onread(char *line, void *);
static void onterm(void *);

int main(int argc, char *argv[])
{
    argl_parse(&argl, argc, argv);
    if (argl_nargs(&argl)) argl_usage(&argl);
    if (argl_has(&argl, "daemon")) daemonize();

    zlog_setup(argl_get(&argl, "logstream"),
               argl_get(&argl, "loglevel")[0] - '0');

    looper_init(&schedy.looper, &onlooper_term, &schedy);

    loopio_init(&schedy.loopio, schedy.looper.loop, &onread, &oneof, &onerror,
                &onterm, &schedy);
    loopio_open(&schedy.loopio, argl_get(&argl, "input"),
                argl_get(&argl, "output"));

    looper_run(&schedy.looper);
    looper_cleanup(&schedy.looper);

    return EXIT_SUCCESS;
}

static void onlooper_term(void *arg)
{
    struct schedy *schedy = arg;
    loopio_terminate(&schedy->loopio);
    cmd_parse(&cmd, "cancel");
    (*cmd_fn(cmd.argv[0]))(&cmd);
}

static void oneof(void *arg)
{
    struct schedy *schedy = arg;
    looper_terminate(&schedy->looper);
}

static void onerror(void *arg)
{
    struct schedy *schedy = arg;
    looper_terminate(&schedy->looper);
}

static void onread(char *line, void *arg)
{
    struct schedy *schedy = arg;
    if (!cmd_parse(&cmd, line)) eparse("too many arguments");
    char const *msg = (*cmd_fn(cmd.argv[0]))(&cmd);
    loopio_put(&schedy->loopio, msg);
}

static void onterm(void *arg)
{
    struct schedy *schedy = arg;
    looper_terminate(&schedy->looper);
}

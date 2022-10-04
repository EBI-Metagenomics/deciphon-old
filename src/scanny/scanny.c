#include "scanny/scanny.h"
#include "argless.h"
#include "core/daemonize.h"
#include "core/logy.h"
#include "scanny/cmd.h"
#include "scanny/session.h"
#include <stdlib.h>

struct scanny scanny = {0};
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
                           .doc = "Scanny program.",
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

    info("starting %s", argl_program(&argl));
    looper_init(&scanny.looper, &onlooper_term, &scanny);

    loopio_init(&scanny.loopio, &scanny.looper.loop, &onread, &oneof, &onerror,
                &onterm, &scanny);
    loopio_open(&scanny.loopio, argl_get(&argl, "input"),
                argl_get(&argl, "output"));

    session_init(&scanny.looper.loop);
    looper_run(&scanny.looper);
    looper_cleanup(&scanny.looper);

    return EXIT_SUCCESS;
}

static void onlooper_term(void *arg)
{
    struct scanny *scanny = arg;
    loopio_terminate(&scanny->loopio);
    cmd_parse(&cmd, "cancel");
    (*cmd_fn(cmd.argv[0]))(&cmd);
}

static void oneof(void *arg)
{
    info("returned end of file");
    struct scanny *scanny = arg;
    looper_terminate(&scanny->looper);
}

static void onerror(void *arg)
{
    struct scanny *scanny = arg;
    looper_terminate(&scanny->looper);
}

static void onread(char *line, void *arg)
{
    struct scanny *scanny = arg;
    if (!cmd_parse(&cmd, line)) eparse("too many arguments");
    loopio_put(&scanny->loopio, (*cmd_fn(cmd.argv[0]))(&cmd));
}

static void onterm(void *arg)
{
    struct scanny *scanny = arg;
    looper_terminate(&scanny->looper);
}

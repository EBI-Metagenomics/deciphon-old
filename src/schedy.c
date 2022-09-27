#include "schedy.h"
#include "argless.h"
#include "core/logging.h"
#include "sched.h"
#include "schedy_cmd.h"
#include <stdlib.h>

struct schedy schedy = {0};

static struct argl_option const options[] = {
    {"input", 'i', "INPUT", "Input stream. Defaults to `STDIN'.",
     ARGL_HASVALUE},
    {"output", 'o', "OUTPUT", "Output stream. Defaults to `STDOUT'.",
     ARGL_HASVALUE},
    {"userlog", 'u', "USERLOG", "User logging stream. Defaults to `STDERR'.",
     ARGL_HASVALUE},
    {"syslog", 's', "SYSLOG", "System logging stream. Defaults to `STDERR'.",
     ARGL_HASVALUE},
    ARGL_DEFAULT_OPTS,
    ARGL_NULL_OPT,
};

static struct argl argl = {.options = options,
                           .args_doc = nullptr,
                           .doc = "Schedy program.",
                           .version = "1.0.0"};

static void onerror(void *);
static void onread(char *line, void *);
static void onterm(void *);

int main(int argc, char *argv[])
{
    argl_parse(&argl, argc, argv);
    if (argl_nargs(&argl)) argl_usage(&argl);
    logging_set_prefix(argl_program(&argl));
    logging_set_user_file(argl_grab(&argl, "userlog", LOGGING_DEFAULT_FILE));
    logging_set_sys_file(argl_grab(&argl, "syslog", LOGGING_DEFAULT_FILE));

    if (setenv("UV_THREADPOOL_SIZE", "1", true))
        warn("failed to set UV_THREADPOOL_SIZE=1");

    looper_init(&schedy.looper, &onterm, &schedy);

    loopio_init(&schedy.loopio, schedy.looper.loop, onread, onerror, &schedy);
    loopio_open(&schedy.loopio, argl_grab(&argl, "input", "&1"),
                argl_grab(&argl, "output", "&2"));

    looper_run(&schedy.looper);
    looper_cleanup(&schedy.looper);

    logging_cleanup();
    return EXIT_SUCCESS;
}

static void onerror(void *arg)
{
    struct schedy *schedy = arg;
    looper_terminate(&schedy->looper);
}

static void onread(char *line, void *arg)
{
    info("Received: %s", line);
    struct schedy *schedy = arg;
    static struct cmd gc = {0};
    if (!cmd_parse(&gc, line)) error("too many arguments");
    loopio_put(&schedy->loopio, (*schedy_cmd(gc.argv[0]))(&gc));
}

static void onterm(void *arg)
{
    struct schedy *schedy = arg;
    loopio_terminate(&schedy->loopio);
    struct cmd cmd = {0};
    cmd_parse(&cmd, "CANCEL");
    schedy_cmd_cancel(&cmd);
}

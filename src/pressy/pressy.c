#include "pressy/pressy.h"
#include "argless.h"
#include "core/logging.h"
#include "pressy/cmd.h"
#include "pressy/session.h"
#include <stdlib.h>

struct pressy pressy = {0};

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
                           .doc = "Pressy program.",
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
    logging_set_prefix(argl_program(&argl));
    logging_set_user_file(argl_grab(&argl, "userlog", LOGGING_DEFAULT_FILE));
    logging_set_sys_file(argl_grab(&argl, "syslog", LOGGING_DEFAULT_FILE));

    if (setenv("UV_THREADPOOL_SIZE", "1", true))
        warn("failed to set UV_THREADPOOL_SIZE=1");

    looper_init(&pressy.looper, &onlooper_term, &pressy);

    loopio_init(&pressy.loopio, pressy.looper.loop, &onread, &oneof, &onerror,
                &onterm, &pressy);
    loopio_open(&pressy.loopio, argl_grab(&argl, "input", "&1"),
                argl_grab(&argl, "output", "&2"));

    pressy_session_init(pressy.looper.loop);
    looper_run(&pressy.looper);
    looper_cleanup(&pressy.looper);

    logging_cleanup();
    return EXIT_SUCCESS;
}

static void onlooper_term(void *arg)
{
    struct pressy *pressy = arg;
    loopio_terminate(&pressy->loopio);
    struct cmd cmd = {0};
    cmd_parse(&cmd, "CANCEL");
    pressy_cmd_cancel(&cmd);
}

static void oneof(void *arg)
{
    struct pressy *pressy = arg;
    looper_terminate(&pressy->looper);
}

static void onerror(void *arg)
{
    struct pressy *pressy = arg;
    looper_terminate(&pressy->looper);
}

static void onread(char *line, void *arg)
{
    struct pressy *pressy = arg;
    static struct cmd cmd = {0};
    if (!cmd_parse(&cmd, line)) eparse("too many arguments");
    loopio_put(&pressy->loopio, (*pressy_cmd(cmd.argv[0]))(&cmd));
}

static void onterm(void *arg)
{
    struct pressy *pressy = arg;
    looper_terminate(&pressy->looper);
}

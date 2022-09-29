#include "scanny/scanny.h"
#include "argless.h"
#include "core/logging.h"
#include "scanny/cmd.h"
#include "scanny/session.h"
#include <stdlib.h>

struct scanny scanny = {0};
static struct cmd cmd = {0};

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
    logging_set_prefix(argl_program(&argl));
    logging_set_user_file(argl_grab(&argl, "userlog", LOGGING_DEFAULT_FILE));
    logging_set_sys_file(argl_grab(&argl, "syslog", LOGGING_DEFAULT_FILE));

    looper_init(&scanny.looper, &onlooper_term, &scanny);

    loopio_init(&scanny.loopio, scanny.looper.loop, &onread, &oneof, &onerror,
                &onterm, &scanny);
    loopio_open(&scanny.loopio, argl_grab(&argl, "input", "&1"),
                argl_grab(&argl, "output", "&2"));

    session_init(scanny.looper.loop);
    looper_run(&scanny.looper);
    looper_cleanup(&scanny.looper);

    logging_cleanup();
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

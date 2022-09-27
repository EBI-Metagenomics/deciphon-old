#include "decy/decy.h"
#include "argless.h"
#include "core/logging.h"
#include "decy/schedy.h"
#include <stdlib.h>

struct decy decy = {0};

static struct argl_option const options[] = {
    {"userlog", 'u', "USERLOG", "User logging stream. Defaults to `STDERR'.",
     ARGL_HASVALUE},
    {"syslog", 's', "SYSLOG", "System logging stream. Defaults to `STDERR'.",
     ARGL_HASVALUE},
    ARGL_DEFAULT_OPTS,
    ARGL_NULL_OPT,
};

static struct argl argl = {.options = options,
                           .args_doc = nullptr,
                           .doc = "Decy program.",
                           .version = "1.0.0"};

static void onlooper_term(void *);
static void onschedy_term(void *);
static void onschedy_connection(char *line, void *);
static void onschedy_online(char *line, void *);

int main(int argc, char *argv[])
{
    argl_parse(&argl, argc, argv);
    if (argl_nargs(&argl)) argl_usage(&argl);
    logging_set_prefix(argl_program(&argl));
    logging_set_user_file(argl_grab(&argl, "userlog", LOGGING_DEFAULT_FILE));
    logging_set_sys_file(argl_grab(&argl, "syslog", LOGGING_DEFAULT_FILE));

    if (setenv("UV_THREADPOOL_SIZE", "1", true))
        warn("failed to set UV_THREADPOOL_SIZE=1");

    looper_init(&decy.looper, &onlooper_term, &decy);

    decy_schedy_init(decy.looper.loop, onschedy_term, nullptr);
    decy_schedy_connect("connect http://127.0.0.1:49329 change-me",
                        &onschedy_connection);
    looper_run(&decy.looper);
    looper_cleanup(&decy.looper);

    logging_cleanup();
    return EXIT_SUCCESS;
}

static void onlooper_term(void *arg)
{
    (void)arg;
    info("%s", __FUNCTION__);
    if (!decy_schedy_isterminated()) decy_schedy_terminate();
}

static void onschedy_term(void *arg)
{
    (void)arg;
    info("%s", __FUNCTION__);
    if (decy_schedy_isterminated()) looper_terminate(&decy.looper);
}

static void onschedy_connection(char *line, void *arg)
{
    (void)arg;
    info("%s: %s", __FUNCTION__, line);
    if (!strcmp(line, "OK"))
        decy_schedy_is_online(&onschedy_online);
    else
    {
        efail("failed to connect: %s", line);
        decy_schedy_terminate();
    }
}

static void onschedy_online(char *line, void *arg)
{
    (void)arg;
    if (!strcmp(line, "YES")) return;
    if (!strcmp(line, "NO"))
    {
        efail("offline");
        decy_schedy_terminate();
        return;
    }
    eparse("unexpected reply: %s", line);
    decy_schedy_terminate();
}

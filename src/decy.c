#include "decy.h"
#include "argless.h"
#include "core/logging.h"
#include "decy_session.h"
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

static inline char const *get(char const *name, char const *default_value)
{
    return argl_has(&argl, name) ? argl_get(&argl, name) : default_value;
}

static void onterm(void *);

int main(int argc, char *argv[])
{
    argl_parse(&argl, argc, argv);
    if (argl_nargs(&argl)) argl_usage(&argl);
    logging_set_prefix(argl_program(&argl));
    logging_set_user_file(get("userlog", LOGGING_DEFAULT_FILE));
    logging_set_sys_file(get("syslog", LOGGING_DEFAULT_FILE));

    looper_init(&decy.looper, &onterm, &decy);

    decy_session_init(decy.looper.loop);
    looper_run(&decy.looper);
    looper_cleanup(&decy.looper);

    logging_cleanup();
    return EXIT_SUCCESS;
}

static void onterm(void *arg)
{
    struct decy *decy = arg;
    info("Ponto 1");
    decy_session_cleanup();
}

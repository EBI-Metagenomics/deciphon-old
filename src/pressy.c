#include "pressy.h"
#include "argless.h"
#include "core/logging.h"
#include "pressy_cmd.h"
#include "pressy_session.h"
#include <stdlib.h>

struct pressy pressy = {0};

static struct argl_option const options[] = {
    {"input", 'i', "INPUT", "Input stream. Defaults to `STDIN'.", false},
    {"output", 'o', "OUTPUT", "Output stream. Defaults to `STDOUT'.", false},
    {"userlog", 'u', "USERLOG", "User logging stream. Defaults to `STDERR'.",
     false},
    {"syslog", 's', "SYSLOG", "System logging stream. Defaults to `STDERR'.",
     false},
    ARGL_DEFAULT_OPTS,
    ARGL_NULL_OPT,
};

static struct argl argl = {.options = options,
                           .args_doc = nullptr,
                           .doc = "Pressy program.",
                           .version = "1.0.0"};

static inline char const *get(char const *name, char const *default_value)
{
    return argl_has(&argl, name) ? argl_get(&argl, name) : default_value;
}

static void onread(char *line, void *);
static void onterm(void *);

int main(int argc, char *argv[])
{
    argl_parse(&argl, argc, argv);
    if (argl_nargs(&argl)) argl_usage(&argl);
    logging_set_user_file(get("userlog", LOGGING_DEFAULT_FILE));
    logging_set_sys_file(get("syslog", LOGGING_DEFAULT_FILE));

    if (setenv("UV_THREADPOOL_SIZE", "1", true))
        warn("failed to set UV_THREADPOOL_SIZE=1");

    looper_init(&pressy.looper, &onterm, &pressy);

    loopio_init(&pressy.loopio, &pressy.looper, onread, &pressy);
    loopio_iopen(&pressy.loopio, get("input", "&1"), 0);
    loopio_oopen(&pressy.loopio, get("output", "&2"), UV_FS_O_CREAT);

    pressy_session_init(pressy.looper.loop);
    looper_run(&pressy.looper);
    looper_cleanup(&pressy.looper);

    logging_cleanup();
    return EXIT_SUCCESS;
}

static void onread(char *line, void *arg)
{
    info("Received: %s", line);
    struct pressy *pressy = arg;
    static struct cmd cmd = {0};
    if (!cmd_parse(&cmd, line)) error("too many arguments");
    loopio_put(&pressy->loopio, (*pressy_cmd(cmd.argv[0]))(&cmd));
}

static void onterm(void *arg)
{
    struct pressy *pressy = arg;
    loopio_terminate(&pressy->loopio);
    struct cmd cmd = {0};
    cmd_parse(&cmd, "CANCEL");
    pressy_cmd_cancel(&cmd);
}

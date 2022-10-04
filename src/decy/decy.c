#include "decy/decy.h"
#include "argless.h"
#include "core/daemonize.h"
#include "core/fmt.h"
#include "core/logy.h"
#include "decy/cfg.h"
#include "decy/schedy.h"
#include <stdlib.h>

struct decy decy = {0};

static struct argl_option const options[] = {
    {"logstream", 's', ARGL_TEXT("LOGSTREAM", "&3"), "Logging stream."},
    {"loglevel", 'l', ARGL_TEXT("LOGLEVEL", "2"), "Logging level."},
    {"pid", 'p', ARGL_TEXT("PIDFILE", ARGL_NULL), "PID file."},
    {"daemon", 'D', ARGL_FLAG(), "Daemonize this program."},
    ARGL_DEFAULT,
    ARGL_END,
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
    if (argl_has(&argl, "daemon")) daemonize();

    zlog_setup(argl_get(&argl, "logstream"),
               argl_get(&argl, "loglevel")[0] - '0');
    cfg_init();

    looper_init(&decy.looper, &onlooper_term, &decy);

    schedy_init(&decy.looper.loop, onschedy_term, nullptr);
    schedy_setup(cfg_uri(), cfg_key(), &onschedy_connection);

    looper_run(&decy.looper);
    looper_cleanup(&decy.looper);

    return EXIT_SUCCESS;
}

static void onlooper_term(void *arg)
{
    (void)arg;
    info("%s", __FUNCTION__);
    if (!schedy_isterminated()) schedy_terminate();
}

static void onschedy_term(void *arg)
{
    (void)arg;
    info("%s", __FUNCTION__);
    if (schedy_isterminated()) looper_terminate(&decy.looper);
}

static void onschedy_connection(char *line, void *arg)
{
    (void)arg;
    info("%s: %s", __FUNCTION__, line);
    if (!strcmp(line, "OK"))
        schedy_is_online(&onschedy_online);
    else
    {
        efail("failed to connect: %s", line);
        schedy_terminate();
    }
}

static void onschedy_online(char *line, void *arg)
{
    (void)arg;
    if (!strcmp(line, "YES")) return;
    if (!strcmp(line, "NO"))
    {
        efail("offline");
        schedy_terminate();
        return;
    }
    eparse("unexpected reply: %s", line);
    schedy_terminate();
}

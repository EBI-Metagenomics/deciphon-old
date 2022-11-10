#include "argless.h"
#include "broker.h"
#include "cfg.h"
#include "command.h"
#include "core/as.h"
#include "core/global.h"
#include "core/is.h"
#include "core/logy.h"
#include "core/pidfile.h"

static struct argl_option const options[] = {
    {"loglevel", 'L', ARGL_TEXT("LOGLEVEL", "0"), "Logging level."},
    {"pid", 'p', ARGL_TEXT("PIDFILE", ARGL_NULL), "PID file."},
    {"polling", 'r', ARGL_TEXT("RATE", "1"),
     "Pending job polling frequency per second."},
    ARGL_DEFAULT,
    ARGL_END,
};

static struct argl argl = {.options = options,
                           .args_doc = NULL,
                           .doc = "Decy program.",
                           .version = "1.0.0"};

static void on_term(void) { broker_terminate(); }
static bool on_linger(void) { return false; }
static void on_exit(void) {}

int main(int argc, char *argv[])
{
    global_init(&on_term, &on_linger, &on_exit, argv[0]);

    argl_parse(&argl, argc, argv);
    if (argl_nargs(&argl)) argl_usage(&argl);
    if (argl_has(&argl, "pid")) pidfile_save(argl_get(&argl, "pid"));

    char const *polling = argl_get(&argl, "polling");
    if (!is_long(polling) || as_long(polling) < 0) argl_usage(&argl);
    long repeat = as_long(polling) > 1000 ? 1000 : as_long(polling);
    repeat = repeat < 0 ? 0 : repeat;
    repeat = 1000 / repeat + 1000 % repeat;

    global_setlog(argl_get(&argl, "loglevel")[0] - '0');
    cfg_init();
    broker_init(repeat, cfg_uri(), cfg_key());

    return global_run();
}

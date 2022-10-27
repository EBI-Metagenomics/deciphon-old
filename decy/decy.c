#include "decy.h"
#include "argless.h"
#include "broker.h"
#include "cfg.h"
#include "command.h"
#include "core/as.h"
#include "core/global.h"
#include "core/is.h"
#include "core/logy.h"
#include "core/msg.h"
#include "core/pidfile.h"

struct parent parent = {0};

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

static void on_term(void);

int main(int argc, char *argv[])
{
    argl_parse(&argl, argc, argv);
    if (argl_nargs(&argl)) argl_usage(&argl);
    if (argl_has(&argl, "pid")) pidfile_save(argl_get(&argl, "pid"));
    int loglvl = argl_get(&argl, "loglevel")[0] - '0';

    char const *polling = argl_get(&argl, "polling");
    if (!is_int64(polling) || as_int64(polling) < 0) argl_usage(&argl);
    int64_t repeat = as_int64(polling) > 0
                         ? 1000 / as_int64(polling) + 1000 % as_int64(polling)
                         : 0;

    global_init(on_term, argv[0], loglvl);

    cfg_init();

    parent_init(&parent, &on_read, &terminate, &terminate);
    parent_open(&parent);

    broker_init(repeat);

    global_run();
    global_cleanup();

    return EXIT_SUCCESS;
}

void on_read(char *line)
{
    static struct msg msg = {0};
    if (msg_parse(&msg, line)) return;

    cmd_fn_t *cmd_fn = cmd_get_fn(msg_cmd(&msg));
    if (!cmd_fn) return;

    (*cmd_fn)(&msg);
}

void terminate(void) { global_terminate(); }

static void on_term(void)
{
    // presser_cancel(2500);
    parent_close(&parent);
    broker_terminate();
}

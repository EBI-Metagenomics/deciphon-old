#include "scanny.h"
#include "argless.h"
#include "command.h"
#include "core/global.h"
#include "core/logy.h"
#include "core/msg.h"
#include "core/pidfile.h"
#include "loop/parent.h"
#include "scanner.h"

struct parent parent = {0};

static struct argl_option const options[] = {
    {"loglevel", 'L', ARGL_TEXT("LOGLEVEL", "0"), "Logging level."},
    {"pid", 'p', ARGL_TEXT("PIDFILE", ARGL_NULL), "PID file."},
    ARGL_DEFAULT,
    ARGL_END,
};

static struct argl argl = {.options = options,
                           .args_doc = NULL,
                           .doc = "Scanny program.",
                           .version = "1.0.0"};

static void on_read(char *line);
static void on_term(void);
static bool on_linger(void) { return false; }
static void on_exit(void);

static void terminate(void) { global_terminate(); }

int main(int argc, char *argv[])
{
    global_init(&on_term, &on_linger, &on_exit, argv[0]);

    argl_parse(&argl, argc, argv);
    if (argl_nargs(&argl)) argl_usage(&argl);
    if (argl_has(&argl, "pid")) pidfile_save(argl_get(&argl, "pid"));

    global_setlog(argl_get(&argl, "loglevel")[0] - '0');
    parent_init(&parent, &on_read, &terminate, &terminate, &terminate);
    parent_start(&parent);
    scanner_init();

    return global_run();
}

static void on_read(char *line)
{
    static struct msg msg = {0};
    if (msg_parse(&msg, line)) return;

    cmd_fn_t *cmd_fn = cmd_get_fn(msg_cmd(&msg));
    if (!cmd_fn) return;

    (*cmd_fn)(&msg);
}

static void on_term(void)
{
    scanner_cancel(2500);
    parent_stop(&parent);
}

static void on_exit(void)
{
    scanner_cleanup();
    parent_cleanup(&parent);
}

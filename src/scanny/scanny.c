#include "scanny/scanny.h"
#include "argless.h"
#include "core/c23.h"
#include "core/daemonize.h"
#include "core/global.h"
#include "core/logy.h"
#include "core/pp.h"
#include "core/str.h"
#include "scanny/cmd.h"
#include "scanny/session.h"
#include <stdlib.h>

struct input input = {0};
struct output output = {0};
static struct cmd cmd = {0};

static struct argl_option const options[] = {
    {"loglevel", 'L', ARGL_TEXT("LOGLEVEL", "0"), "Logging level."},
    {"pid", 'p', ARGL_TEXT("PIDFILE", ARGL_NULL), "PID file."},
    {"daemon", 'D', ARGL_FLAG(), "Daemonize this program."},
    ARGL_DEFAULT,
    ARGL_END,
};

static struct argl argl = {.options = options,
                           .args_doc = nullptr,
                           .doc = "Scanny program.",
                           .version = "1.0.0"};

static void on_eof(void *arg);
static void on_read_error(void *arg);
static void on_read(char *line, void *);
static void on_write_error(void *arg);
static void on_term(void);

static void myprint(char const *string, void *arg) { fputs(string, arg); }

int main(int argc, char *argv[])
{
    global_init(on_term, argc, argv);

    argl_parse(&argl, argc, argv);
    if (argl_nargs(&argl)) argl_usage(&argl);
    if (argl_has(&argl, "daemon")) daemonize(true, true, false, true);

    zlog_setup(myprint, stderr, argl_get(&argl, "loglevel")[0] - '0');

    session_init();

    global_run();
    global_cleanup();

    return EXIT_SUCCESS;
}

static void on_eof(void *arg)
{
    UNUSED(arg);
    warn("%s", __FUNCTION__);
    global_terminate();
}

static void on_read_error(void *arg)
{
    UNUSED(arg);
    warn("%s", __FUNCTION__);
    global_terminate();
}

static void on_read(char *line, void *arg)
{
    UNUSED(arg);
    if (str_all_spaces(line)) return;
    if (!cmd_parse(&cmd, line)) eparse("too many arguments");
    output_put(&output, (*cmd_fn(cmd.argv[0]))(&cmd));
}

static void on_write_error(void *arg)
{
    UNUSED(arg);
    warn("%s", __FUNCTION__);
    global_terminate();
}

static void on_term(void)
{
    cmd_parse(&cmd, "cancel");
    (*cmd_fn(cmd.argv[0]))(&cmd);
    input_close(&input);
    output_close(&output);
}

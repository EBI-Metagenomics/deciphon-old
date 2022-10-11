#include "decy/decy.h"
#include "argless.h"
#include "core/c23.h"
#include "core/fmt.h"
#include "core/global.h"
#include "core/logy.h"
#include "core/pidfile.h"
#include "core/pp.h"
#include "core/str.h"
#include "core/xmem.h"
#include "decy/cfg.h"
#include "decy/cmd.h"
#include "decy/schedy.h"
#include "loop/child.h"
#include <stdlib.h>

struct input input = {0};
struct output output = {0};
enum target target = TARGET_DECY;
static struct cmd cmd = {0};
static struct child pressy = {0};
static struct child schedy = {0};
static struct child scanny = {0};

static struct argl_option const options[] = {
    {"loglevel", 'L', ARGL_TEXT("LOGLEVEL", "0"), "Logging level."},
    {"pid", 'p', ARGL_TEXT("PIDFILE", ARGL_NULL), "PID file."},
    ARGL_DEFAULT,
    ARGL_END,
};

static struct argl argl = {.options = options,
                           .args_doc = nullptr,
                           .doc = "Decy program.",
                           .version = "1.0.0"};

// static void onlooper_term(void *);
// static void onschedy_term(void *);
// static void onschedy_connection(char *line, void *);
// static void onschedy_online(char *line, void *);

static void on_eof(void *arg);
static void on_read_error(void *arg);
static void on_read(char *line, void *);
static void on_write_error(void *arg);
static void on_term(void);

static void myprint(char const *string, void *arg) { fputs(string, arg); }

static void on_pressy_exit(void) { global_terminate(); }
static void on_pressy_eof(void *arg) { UNUSED(arg); }
static void on_pressy_read_error(void *arg) { UNUSED(arg); }
static void on_pressy_write_error(void *arg) { UNUSED(arg); }
static void on_pressy_read(char *line, void *arg)
{
    UNUSED(arg);
    static char string[512] = {0};
    snprintf(string, sizeof string, "%s", line);
    output_put(&output, string);
}

static void on_schedy_exit(void) { global_terminate(); }
static void on_schedy_eof(void *arg) { UNUSED(arg); }
static void on_schedy_read_error(void *arg) { UNUSED(arg); }
static void on_schedy_write_error(void *arg) { UNUSED(arg); }
static void on_schedy_read(char *line, void *arg)
{
    UNUSED(arg);
    static char string[512] = {0};
    snprintf(string, sizeof string, "%s", line);
    output_put(&output, string);
}

static void on_scanny_exit(void) { global_terminate(); }
static void on_scanny_eof(void *arg) { UNUSED(arg); }
static void on_scanny_read_error(void *arg) { UNUSED(arg); }
static void on_scanny_write_error(void *arg) { UNUSED(arg); }
static void on_scanny_read(char *line, void *arg)
{
    UNUSED(arg);
    static char string[512] = {0};
    snprintf(string, sizeof string, "%s", line);
    output_put(&output, string);
}

int main(int argc, char *argv[])
{
    global_init(on_term, argc, argv);

    argl_parse(&argl, argc, argv);
    if (argl_nargs(&argl)) argl_usage(&argl);
    if (argl_has(&argl, "pid")) pidfile_save(argl_get(&argl, "pid"));

    zlog_setup(myprint, stderr, argl_get(&argl, "loglevel")[0] - '0');

    cfg_init();

    info("starting %s", argl_program(&argl));
    input_init(&input, STDIN_FILENO);
    input_cb(&input)->on_eof = &on_eof;
    input_cb(&input)->on_error = &on_read_error;
    input_cb(&input)->on_read = &on_read;
    input_cb(&input)->arg = NULL;
    input_start(&input);

    output_init(&output, STDOUT_FILENO);
    output_cb(&output)->on_error = &on_write_error;
    output_cb(&output)->arg = NULL;
    output_start(&output);

    child_init(&pressy);
    child_cb(&pressy)->on_exit = &on_pressy_exit;
    child_cb(&pressy)->arg = NULL;
    child_input_cb(&pressy)->on_eof = &on_pressy_eof;
    child_input_cb(&pressy)->on_error = &on_pressy_read_error;
    child_input_cb(&pressy)->on_read = &on_pressy_read;
    child_input_cb(&pressy)->arg = NULL;
    child_output_cb(&pressy)->on_error = &on_pressy_write_error;
    child_output_cb(&pressy)->arg = NULL;
    static char *pressy_args[] = {"./pressy", "--pid", "pressy.pid", NULL};
    child_spawn(&pressy, pressy_args);

    child_init(&schedy);
    child_cb(&schedy)->on_exit = &on_schedy_exit;
    child_cb(&schedy)->arg = NULL;
    child_input_cb(&schedy)->on_eof = &on_schedy_eof;
    child_input_cb(&schedy)->on_error = &on_schedy_read_error;
    child_input_cb(&schedy)->on_read = &on_schedy_read;
    child_input_cb(&schedy)->arg = NULL;
    child_output_cb(&schedy)->on_error = &on_schedy_write_error;
    child_output_cb(&schedy)->arg = NULL;
    static char *schedy_args[] = {"./schedy", "--pid", "schedy.pid", NULL};
    child_spawn(&schedy, schedy_args);

    child_init(&scanny);
    child_cb(&scanny)->on_exit = &on_scanny_exit;
    child_cb(&scanny)->arg = NULL;
    child_input_cb(&scanny)->on_eof = &on_scanny_eof;
    child_input_cb(&scanny)->on_error = &on_scanny_read_error;
    child_input_cb(&scanny)->on_read = &on_scanny_read;
    child_input_cb(&scanny)->arg = NULL;
    child_output_cb(&scanny)->on_error = &on_scanny_write_error;
    child_output_cb(&scanny)->arg = NULL;
    static char *scanny_args[] = {"./scanny", "--pid", "scanny.pid", NULL};
    child_spawn(&scanny, scanny_args);

    global_run();
    global_cleanup();

    // schedy_init(&decy.looper.loop, onschedy_term, nullptr);
    // schedy_setup(cfg_uri(), cfg_key(), &onschedy_connection);
    //
    // looper_run(&decy.looper);
    // looper_cleanup(&decy.looper);

    return EXIT_SUCCESS;
}

#if 0
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
#endif

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

    char *saved = xmemdup(line, strlen(line) + 1);
    UNUSED(arg);
    if (str_all_spaces(line)) return;
    if (!cmd_parse(&cmd, line)) eparse("too many arguments");
    if (!strcmp(cmd_get(&cmd, 0), "target"))
        output_put(&output, (*cmd_fn(cmd.argv[0]))(&cmd));
    else if (target == TARGET_DECY)
        output_put(&output, (*cmd_fn(cmd.argv[0]))(&cmd));
    else if (target == TARGET_PRESSY)
        child_send(&pressy, saved);
    else if (target == TARGET_SCHEDY)
        child_send(&schedy, saved);
    else if (target == TARGET_SCANNY)
        child_send(&scanny, saved);
    else
        error("unknown target");
}

static void on_write_error(void *arg)
{
    UNUSED(arg);
    warn("%s", __FUNCTION__);
    global_terminate();
}

static void on_term(void)
{
    input_close(&input);
    output_close(&output);
}

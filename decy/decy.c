#include "decy.h"
#include "argless.h"
#include "cfg.h"
#include "core/c23.h"
#include "core/fmt.h"
#include "core/global.h"
#include "core/logy.h"
#include "core/pidfile.h"
#include "core/pp.h"
#include "core/str.h"
#include "core/xmem.h"
#include "msg.h"
#include "session.h"
#include <stdlib.h>

struct input input = {0};
struct output output = {0};
enum target target = TARGET_DECY;
static struct msg msg = {0};

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

static void on_eof(void *arg);
static void on_read_error(void *arg);
static void on_read(char *line, void *);
static void on_write_error(void *arg);
static void on_term(void);

int main(int argc, char *argv[])
{
    argl_parse(&argl, argc, argv);
    if (argl_nargs(&argl)) argl_usage(&argl);
    if (argl_has(&argl, "pid")) pidfile_save(argl_get(&argl, "pid"));
    global_init(on_term, argc, argv, argl_get(&argl, "loglevel")[0] - '0');

    cfg_init();

    input_init(&input, STDIN_FILENO);
    input_setup(&input, &on_eof, &on_read_error, &on_read, NULL);
    input_start(&input);

    output_init(&output, STDOUT_FILENO);
    output_setup(&output, &on_write_error, NULL);
    output_start(&output);

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
    debug("DECY: %s", __FUNCTION__);
    UNUSED(arg);
    if (str_all_spaces(line)) return;
    if (!msg_parse(&msg, line)) eparse("too many arguments");
    output_put(&output, (*msg_fn(msg.cmd.argv[0]))(&msg));
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
    session_terminate();
}

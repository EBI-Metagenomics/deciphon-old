#include "scanny.h"
#include "argless.h"
#include "core/c23.h"
#include "core/global.h"
#include "core/logy.h"
#include "core/pidfile.h"
#include "core/pp.h"
#include "core/str.h"
#include "msg.h"
#include "session.h"
#include <stdlib.h>

struct input input = {0};
struct output output = {0};
static struct msg msg = {0};

static struct argl_option const options[] = {
    {"loglevel", 'L', ARGL_TEXT("LOGLEVEL", "0"), "Logging level."},
    {"pid", 'p', ARGL_TEXT("PIDFILE", ARGL_NULL), "PID file."},
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
    if (argl_has(&argl, "pid")) pidfile_save(argl_get(&argl, "pid"));

    zlog_setup(myprint, stderr, argl_get(&argl, "loglevel")[0] - '0');

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
    msg_parse(&msg, "cancel");
    (*msg_fn(msg.cmd.argv[0]))(&msg);
    input_close(&input);
    output_close(&output);
}

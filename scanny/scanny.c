#include "scanny.h"
#include "argless.h"
#include "core/c23.h"
#include "core/global.h"
#include "core/logy.h"
#include "core/pidfile.h"
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

static void on_eof(void);
static void on_read_error(void);
static void on_read(char *line);
static void on_write_error(void);
static void on_term(void);

int main(int argc, char *argv[])
{
    argl_parse(&argl, argc, argv);
    if (argl_nargs(&argl)) argl_usage(&argl);
    if (argl_has(&argl, "pid")) pidfile_save(argl_get(&argl, "pid"));
    global_init(on_term, argc, argv, argl_get(&argl, "loglevel")[0] - '0');

    input_init(&input, STDIN_FILENO);
    input_setup(&input, &on_eof, &on_read_error, &on_read);
    input_start(&input);

    output_init(&output, STDOUT_FILENO);
    output_setup(&output, &on_write_error);
    output_start(&output);

    session_init();

    global_run();
    global_cleanup();

    return EXIT_SUCCESS;
}

static void on_eof(void)
{
    warn("%s", __FUNCTION__);
    global_terminate();
}

static void on_read_error(void)
{
    warn("%s", __FUNCTION__);
    global_terminate();
}

static void on_read(char *line)
{
    if (!msg_parse(&msg, line)) eparse("too many arguments");
    (*msg_fn(msg.cmd.argv[0]))(&msg);
}

static void on_write_error(void)
{
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

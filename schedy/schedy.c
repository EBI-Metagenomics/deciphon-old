#include "schedy.h"
#include "api.h"
#include "argless.h"
#include "core/global.h"
#include "core/logy.h"
#include "core/pidfile.h"
#include "core/pp.h"
#include "core/str.h"
#include "lazylog.h"
#include "msg.h"
#include <stdlib.h>

struct input input = {0};
struct output output = {0};
static struct msg msg = {0};

#define API_URL "http://127.0.0.1:49329"
#define API_KEY "change-me"

static struct argl_option const options[] = {
    {"url", 'u', ARGL_TEXT("APIURL", API_URL), "API url."},
    {"key", 'k', ARGL_TEXT("APIKEY", API_KEY), "API key."},
    {"loglevel", 'L', ARGL_TEXT("LOGLEVEL", "0"), "Logging level."},
    {"pid", 'p', ARGL_TEXT("PIDFILE", ARGL_NULL), "PID file."},
    ARGL_DEFAULT,
    ARGL_END,
};

static struct argl argl = {.options = options,
                           .args_doc = NULL,
                           .doc = "Schedy program.",
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

    char const *url = argl_get(&argl, "url");
    char const *key = argl_get(&argl, "key");
    if (api_init(url, key)) return EXIT_FAILURE;

    input_init(&input, STDIN_FILENO);
    input_setup(&input, &on_eof, &on_read_error, &on_read, NULL);
    input_start(&input);

    output_init(&output, STDOUT_FILENO);
    output_setup(&output, &on_write_error, NULL);
    output_start(&output);

    global_run();
    global_cleanup();
    api_cleanup();

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
    debug("SCHEDY: %s", __FUNCTION__);
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

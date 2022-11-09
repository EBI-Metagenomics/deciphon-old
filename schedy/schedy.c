#include "schedy.h"
#include "api.h"
#include "argless.h"
#include "command.h"
#include "core/global.h"
#include "core/logy.h"
#include "core/msg.h"
#include "core/pidfile.h"
#include "lazylog.h"
#include "loop/parent.h"
#include <stdlib.h>

struct parent parent = {0};

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

static void on_read(char *line);
static void on_term(void);
static void terminate(void) { global_terminate(); }

int main(int argc, char *argv[])
{
    global_init(on_term, argv[0]);

    argl_parse(&argl, argc, argv);
    if (argl_nargs(&argl)) argl_usage(&argl);
    if (argl_has(&argl, "pid")) pidfile_save(argl_get(&argl, "pid"));
    char const *url = argl_get(&argl, "url");
    char const *key = argl_get(&argl, "key");

    global_setlog(argl_get(&argl, "loglevel")[0] - '0');
    if (api_init(url, key)) global_die();
    parent_init(&parent, &on_read, &terminate, &terminate);
    parent_open(&parent);

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

static void on_term(void) { parent_close(&parent); }

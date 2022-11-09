#include "hmmy.h"
#include "argless.h"
#include "command.h"
#include "core/global.h"
#include "core/logy.h"
#include "core/msg.h"
#include "core/pidfile.h"
#include "fs.h"
#include "hmmer.h"
#include "loop/parent.h"

struct parent parent = {0};

static struct argl_option const options[] = {
    {"loglevel", 'L', ARGL_TEXT("LOGLEVEL", "0"), "Logging level."},
    {"pid", 'p', ARGL_TEXT("PIDFILE", ARGL_NULL), "PID file."},
    {"podman", 'P', ARGL_TEXT("PODMAN", ARGL_NULL), "Podman executable file."},
    ARGL_DEFAULT,
    ARGL_END,
};

static struct argl argl = {.options = options,
                           .args_doc = NULL,
                           .doc = "Hmmy program.",
                           .version = "1.0.0"};

static void on_read(char *line);
static void on_term(void);
static void terminate(void) { global_terminate(); }
static char const *find_podman(void);

int main(int argc, char *argv[])
{
    argl_parse(&argl, argc, argv);
    if (argl_nargs(&argl)) argl_usage(&argl);
    if (argl_has(&argl, "pid")) pidfile_save(argl_get(&argl, "pid"));
    int loglvl = argl_get(&argl, "loglevel")[0] - '0';
    char const *podman =
        argl_has(&argl, "podman") ? argl_get(&argl, "podman") : find_podman();

    global_init(on_term, argv[0], loglvl);
    if (!podman)
    {
        error("Please, provide a valid executable path to podman.");
        return EXIT_FAILURE;
    }

    parent_init(&parent, &on_read, &terminate, &terminate);
    parent_open(&parent);

    hmmer_init(podman);
    global_run();
    return global_cleanup();
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
    debug("aqui");
    hmmer_stop();
    parent_close(&parent);
}

static char const *find_podman(void)
{
    static char const *paths[] = {"/usr/bin/podman", "/opt/homebrew/bin/podman",
                                  NULL};
    char const **p = &paths[0];
    while (p)
    {
        if (fs_exists(*p)) return *p;
        ++p;
    }
    return NULL;
}

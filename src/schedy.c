#include "schedy.h"
#include "argless.h"
#include "core/api.h"
#include "core/logging.h"
#include "sched.h"
#include "schedy_cmd.h"
#include <stdlib.h>

struct schedy schedy = {0};

static struct argl_option const options[] = {
    {"input", 'i', "INPUT", "Input stream. Defaults to `STDIN'.", false},
    {"output", 'o', "OUTPUT", "Output stream. Defaults to `STDOUT'.", false},
    ARGL_DEFAULT_OPTS,
    ARGL_NULL_OPT,
};

static struct argl argl = {.options = options,
                           .args_doc = nullptr,
                           .doc = "Schedy program.",
                           .version = "1.0.0"};

static inline char const *get(char const *name, char const *default_value)
{
    return argl_has(&argl, name) ? argl_get(&argl, name) : default_value;
}

static void onread(char *line, void *);
static void onterm(void *);

int main(int argc, char *argv[])
{
    argl_parse(&argl, argc, argv);
    if (argl_nargs(&argl)) argl_usage(&argl);

    looper_init(&schedy.looper, &onterm, &schedy);

    loopio_init(&schedy.loopio, &schedy.looper, onread, &schedy);
    loopio_iopen(&schedy.loopio, get("input", "&1"), 0);
    loopio_oopen(&schedy.loopio, get("output", "&2"), UV_FS_O_CREAT);

    looper_run(&schedy.looper);
    looper_cleanup(&schedy.looper);

    return EXIT_SUCCESS;
}

static void onread(char *line, void *arg)
{
    struct schedy *schedy = arg;
    static struct cmd gc = {0};
    if (!cmd_parse(&gc, line)) error("too many arguments");
    loopio_put(&schedy->loopio, (*schedy_cmd(gc.argv[0]))(&gc));
}

static void onterm(void *arg)
{
    struct schedy *schedy = arg;
    loopio_terminate(&schedy->loopio);
}

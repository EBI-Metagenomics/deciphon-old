#include "core/global.h"
#include "core/basename.h"
#include "core/die.h"
#include "core/exe.h"
#include "core/run_once.h"
#include "core/strlcpy.h"
#include "core/unused.h"
#include "lazylog.h"
#include "loop/halt_signal.h"
#include "loop/linger.h"
#include <stdio.h>
#include <uv.h>

static char title[128] = {0};
static on_cleanup_fn_t *on_cleanup = NULL;

static void logger(char const *string, void *arg);
static void halt(void);

static void on_cleanup_fwd(void)
{
    halt_signal_close();
    if (on_cleanup) (*on_cleanup)();
}

void global_init(char *const arg0, int loglvl, on_linger_fn_t *linger,
                 on_cleanup_fn_t *cleanup)
{
    zlog_setup(logger, stderr, loglvl);

    on_cleanup = cleanup;
    linger_init(global_loop(), linger, &on_cleanup_fwd);
    halt_signal_open(global_loop(), &halt);
    exe_init();

    strlcpy(title, arg0, sizeof title);
    char *t = basename(title);
    memmove(title, t, strlen(t) + 1);
}

struct uv_loop_s *global_loop(void) { return uv_default_loop(); }

void global_shutdown(void)
{
    RUN_ONCE;
    linger_trigger();
}

int global_run(void)
{
    if (uv_run(global_loop(), UV_RUN_DEFAULT)) die();
    return uv_loop_close(global_loop());
}

static void halt(void)
{
    halt_signal_close();
    linger_cleanup();
}

static void logger(char const *string, void *arg)
{
    FILE *fp = arg;
    fprintf(fp, "[%6s] ", title);
    fputs(string, fp);
}

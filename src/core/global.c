#include "core/global.h"
#include "core/logy.h"
#include "core/pp.h"
#include "uv.h"
#include "zc.h"
#include <stdlib.h>
#include <string.h>

#define LINGER_TIMEOUT 5000L

static struct uv_async_s async = {0};
static struct uv_signal_s sigterm = {0};
static struct uv_signal_s sigint = {0};
static struct uv_timer_s linger_timer = {0};
static long linger_start = 0;
static int remain_handlers = 3;
static on_term_fn_t *on_term_fn = NULL;
static on_linger_fn_t *on_linger_fn = NULL;
static on_exit_fn_t *on_exit_fn = NULL;
static char title[128] = {0};
static char exepath[FILENAME_MAX] = {0};
static char exedir[FILENAME_MAX] = {0};

static void async_cb(struct uv_async_s *handle);
static void sigterm_cb(struct uv_signal_s *handle, int signum);
static void sigint_cb(struct uv_signal_s *handle, int signum);
static void logprinter(char const *string, void *arg);

_Noreturn static void die(void) { exit(EXIT_FAILURE); }

void global_init(on_term_fn_t *on_term, on_linger_fn_t *on_linger,
                 on_exit_fn_t *on_exit, char *const arg0)
{
    global_setlog(ZLOG_NOTSET);
    zc_strlcpy(title, arg0, sizeof title);
    char *t = zc_basename(title);
    memmove(title, t, strlen(t) + 1);
    if (uv_async_init(global_loop(), &async, async_cb)) die();
    if (uv_signal_init(global_loop(), &sigterm)) die();
    if (uv_signal_init(global_loop(), &sigint)) die();
    if (uv_signal_start(&sigterm, sigterm_cb, SIGTERM)) die();
    if (uv_signal_start(&sigint, sigint_cb, SIGINT)) die();
    if (uv_timer_init(global_loop(), &linger_timer)) die();
    remain_handlers = 3;
    on_term_fn = on_term;
    on_linger_fn = on_linger;
    on_exit_fn = on_exit;

    size_t sz = sizeof exepath;
    uv_exepath(exepath, &sz);

    sz = sizeof exedir;
    uv_exepath(exedir, &sz);
    zc_dirname(exedir);

    info("starting");
}

void global_setlog(int log_level) { zlog_setup(logprinter, stderr, log_level); }

char const *global_title(void) { return title; }

char const *global_exepath(void) { return exepath; }

char const *global_exedir(void) { return exedir; }

long global_now(void) { return (long)uv_now(global_loop()); }

struct uv_loop_s *global_loop(void) { return uv_default_loop(); }

void global_terminate(void)
{
    if (uv_async_send(&async)) die();
}

_Noreturn void global_die(void)
{
    uv_loop_close(global_loop());
    die();
}

int global_run(void)
{
    if (uv_run(global_loop(), UV_RUN_DEFAULT)) die();
    return uv_loop_close(global_loop());
}

static void call_exit(struct uv_handle_s *handle)
{
    unused(handle);
    (*on_exit_fn)();
}

static void linger_fwd(struct uv_timer_s *timer)
{
    unused(timer);
    if (global_now() - linger_start > LINGER_TIMEOUT || !(*on_linger_fn)())
    {
        if (uv_timer_stop(&linger_timer)) die();
        uv_close((struct uv_handle_s *)&linger_timer, &call_exit);
        --remain_handlers;
    }
}

static void terminate(struct uv_handle_s *handle)
{
    unused(handle);
    if (!--remain_handlers)
    {
        (*on_term_fn)();
        if (uv_timer_start(&linger_timer, &linger_fwd, 100, 100)) die();
        ++remain_handlers;
        linger_start = global_now();
    }
}

static void async_cb(struct uv_async_s *handle)
{
    unused(handle);
    uv_close((struct uv_handle_s *)&async, &terminate);
    uv_close((struct uv_handle_s *)&sigterm, &terminate);
    uv_close((struct uv_handle_s *)&sigint, &terminate);
}

static void sigterm_cb(struct uv_signal_s *handle, int signum)
{
    unused(handle, signum);
    if (uv_signal_stop(&sigterm)) die();
    if (uv_async_send(&async)) die();
}

static void sigint_cb(struct uv_signal_s *handle, int signum)
{
    unused(handle, signum);
    if (uv_signal_stop(&sigint)) die();
    if (uv_async_send(&async)) die();
}

static void logprinter(char const *string, void *arg)
{
    FILE *fp = arg;
    fprintf(fp, "[%6s] ", global_title());
    fputs(string, fp);
}

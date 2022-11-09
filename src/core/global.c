#include "core/global.h"
#include "core/logy.h"
#include "core/pp.h"
#include "uv.h"
#include "zc.h"
#include <stdlib.h>
#include <string.h>

static struct uv_async_s async = {0};
static struct uv_signal_s sigterm = {0};
static struct uv_signal_s sigint = {0};
static int terminated = 3;
static on_term_fn_t *on_term_fn = NULL;
static char title[128] = {0};
static char exepath[FILENAME_MAX] = {0};
static char exedir[FILENAME_MAX] = {0};

static void async_cb(struct uv_async_s *handle);
static void sigterm_cb(struct uv_signal_s *handle, int signum);
static void sigint_cb(struct uv_signal_s *handle, int signum);
static void logprinter(char const *string, void *arg);

void global_init(on_term_fn_t *on_term, char *const arg0)
{
    global_setlog(ZLOG_NOTSET);
    zc_strlcpy(title, arg0, sizeof title);
    char *t = zc_basename(title);
    memmove(title, t, strlen(t) + 1);
    if (uv_async_init(global_loop(), &async, async_cb)) exit(1);
    if (uv_signal_init(global_loop(), &sigterm)) exit(1);
    if (uv_signal_init(global_loop(), &sigint)) exit(1);
    if (uv_signal_start(&sigterm, sigterm_cb, SIGTERM)) exit(1);
    if (uv_signal_start(&sigint, sigint_cb, SIGINT)) exit(1);
    terminated = 3;
    on_term_fn = on_term;

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
    if (uv_async_send(&async)) exit(EXIT_FAILURE);
}

_Noreturn void global_die(void)
{
    uv_loop_close(global_loop());
    exit(EXIT_FAILURE);
}

int global_run(void)
{
    if (uv_run(global_loop(), UV_RUN_DEFAULT)) exit(EXIT_FAILURE);
    return uv_loop_close(global_loop());
}

static void terminate(struct uv_handle_s *handle)
{
    UNUSED(handle);
    if (!--terminated) (*on_term_fn)();
}

static void async_cb(struct uv_async_s *handle)
{
    UNUSED(handle);
    uv_close((struct uv_handle_s *)&async, &terminate);
    uv_close((struct uv_handle_s *)&sigterm, &terminate);
    uv_close((struct uv_handle_s *)&sigint, &terminate);
}

static void sigterm_cb(struct uv_signal_s *handle, int signum)
{
    UNUSED(handle, signum);
    uv_signal_stop(&sigterm);
    if (uv_async_send(&async)) exit(EXIT_FAILURE);
}

static void sigint_cb(struct uv_signal_s *handle, int signum)
{
    UNUSED(handle, signum);
    uv_signal_stop(&sigint);
    if (uv_async_send(&async)) exit(EXIT_FAILURE);
}

static void logprinter(char const *string, void *arg)
{
    FILE *fp = arg;
    fprintf(fp, "[%6s] ", global_title());
    fputs(string, fp);
}

#include "core/global.h"
#include "core/logy.h"
#include "core/pp.h"
#include "uv.h"
#include "zc.h"
#include <stdlib.h>

static struct uv_async_s async = {0};
static struct uv_signal_s sigterm = {0};
static struct uv_signal_s sigint = {0};
static int terminated = 3;
static on_term_fn_t *on_term_fn = NULL;
static char process_title[128] = {0};

static void async_cb(struct uv_async_s *handle);
static void sigterm_cb(struct uv_signal_s *handle, int signum);
static void sigint_cb(struct uv_signal_s *handle, int signum);
static void logprinter(char const *string, void *arg);

void global_init(on_term_fn_t *on_term, int argc, char *argv[], int log_level)
{
    zlog_setup(logprinter, stderr, log_level);
    uv_setup_args(argc, argv);
    uv_get_process_title(process_title, sizeof process_title);
    char *title = zc_basename(process_title);
    memmove(process_title, title, strlen(title) + 1);
    uv_set_process_title(process_title);
    if (uv_async_init(global_loop(), &async, async_cb)) exit(1);
    if (uv_signal_init(global_loop(), &sigterm)) exit(1);
    if (uv_signal_init(global_loop(), &sigint)) exit(1);
    if (uv_signal_start(&sigterm, sigterm_cb, SIGTERM)) exit(1);
    if (uv_signal_start(&sigint, sigint_cb, SIGINT)) exit(1);
    terminated = 3;
    on_term_fn = on_term;
    info("starting");
}

char const *global_title(void) { return process_title; }

struct uv_loop_s *global_loop(void) { return uv_default_loop(); }

void global_terminate(void)
{
    if (uv_async_send(&async)) exit(1);
}

void global_run(void)
{
    if (uv_run(global_loop(), UV_RUN_DEFAULT)) exit(1);
}

void global_cleanup(void)
{
    if (uv_loop_close(global_loop())) exit(1);
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
    if (uv_async_send(&async)) exit(1);
}

static void sigint_cb(struct uv_signal_s *handle, int signum)
{
    UNUSED(handle, signum);
    uv_signal_stop(&sigint);
    if (uv_async_send(&async)) exit(1);
}

static void logprinter(char const *string, void *arg)
{
    FILE *fp = arg;
    fprintf(fp, "[%6s] ", global_title());
    fputs(string, fp);
}

#include "deciphon/core/looper.h"
#include "deciphon/core/logging.h"
#include "uv.h"
#include <stdbool.h>

static void async_cb(struct uv_async_s *handle);
static void sigterm_cb(struct uv_signal_s *handle, int signum);
static void sigint_cb(struct uv_signal_s *handle, int signum);
static void close_async(struct looper *);
static void close_sigterm(struct looper *);
static void close_sigint(struct looper *);

void looper_init(struct looper *l)
{
    l->terminating = false;

    if (!(l->loop = uv_default_loop())) fatal("uv_default_loop");

    if (uv_async_init(l->loop, &l->async, async_cb)) fatal("uv_async_init");
    if (uv_signal_init(l->loop, &l->sigterm)) fatal("uv_signal_init");
    if (uv_signal_init(l->loop, &l->sigint)) fatal("uv_signal_init");

    l->async.data = l;
    l->sigterm.data = l;
    l->sigint.data = l;

    l->closing.async = false;
    l->closing.sigterm = false;
    l->closing.sigint = false;

    if (uv_signal_start(&l->sigterm, sigterm_cb, SIGTERM))
        fatal("uv_signal_start");

    if (uv_signal_start(&l->sigint, sigint_cb, SIGINT))
        fatal("uv_signal_start");
}

void looper_run(struct looper *l)
{
    if (uv_run(l->loop, UV_RUN_DEFAULT)) fatal("uv_run");
}

void looper_cleanup(struct looper *l)
{
    close_async(l);
    close_sigterm(l);
    close_sigint(l);

    if (uv_loop_close(l->loop)) fatal("uv_loop_close");
}

static void close_async(struct looper *l)
{
    if (!l->closing.async)
    {
        uv_close((struct uv_handle_s *)&l->async, 0);
        l->closing.async = true;
    }
}

static void close_sigterm(struct looper *l)
{
    if (!l->closing.sigterm)
    {
        uv_close((struct uv_handle_s *)&l->sigterm, 0);
        l->closing.sigterm = true;
    }
}

static void close_sigint(struct looper *l)
{
    if (!l->closing.sigint)
    {
        uv_close((struct uv_handle_s *)&l->sigint, 0);
        l->closing.sigint = true;
    }
}

static void close_signals_cb(struct uv_handle_s *handle)
{
    struct looper *l = handle->data;
    close_sigterm(l);
    close_sigint(l);
}

static void async_cb(struct uv_async_s *handle)
{
    struct looper *l = handle->data;
    l->terminating = true;
    if (!l->closing.async)
    {
        uv_close((struct uv_handle_s *)&l->async, close_signals_cb);
        l->closing.async = true;
    }
}

static void please_terminate(struct looper *l)
{
    if (l->terminating) return;
    if (uv_async_send(&l->async)) fatal("uv_async_send");
}

static void sigterm_cb(struct uv_signal_s *handle, int signum)
{
    (void)signum;
    please_terminate(handle->data);
}

static void sigint_cb(struct uv_signal_s *handle, int signum)
{
    (void)signum;
    please_terminate(handle->data);
}

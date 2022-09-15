#include "loop/looper.h"
#include "core/logging.h"
#include "uv.h"
#include <stdbool.h>

static void async_cb(struct uv_async_s *handle);
static void sigterm_cb(struct uv_signal_s *handle, int signum);
static void sigint_cb(struct uv_signal_s *handle, int signum);

void looper_init(struct looper *l, looper_onterm_fn_t *onterm_cb)
{
    l->terminating = false;

    if (!(l->loop = uv_default_loop())) fatal("uv_default_loop");

    if (uv_async_init(l->loop, &l->async, async_cb)) fatal("uv_async_init");
    if (uv_signal_init(l->loop, &l->sigterm)) fatal("uv_signal_init");
    if (uv_signal_init(l->loop, &l->sigint)) fatal("uv_signal_init");

    l->async.data = l;
    l->sigterm.data = l;
    l->sigint.data = l;

    ((struct uv_handle_s *)&l->async)->data = l;
    ((struct uv_handle_s *)&l->sigterm)->data = l;
    ((struct uv_handle_s *)&l->sigint)->data = l;

    l->closing.async = false;
    l->closing.sigterm = false;
    l->closing.sigint = false;

    l->closed.async = false;
    l->closed.sigterm = false;
    l->closed.sigint = false;

    l->onterm_cb = onterm_cb;

    if (uv_signal_start(&l->sigterm, sigterm_cb, SIGTERM))
        fatal("uv_signal_start");

    if (uv_signal_start(&l->sigint, sigint_cb, SIGINT))
        fatal("uv_signal_start");
}

void looper_run(struct looper *l)
{
    if (uv_run(l->loop, UV_RUN_DEFAULT)) fatal("uv_run");
}

void looper_terminate(struct looper *l)
{
    if (l->terminating) return;
    if (uv_async_send(&l->async)) fatal("uv_async_send");
}

void looper_cleanup(struct looper *l)
{
    if (uv_loop_close(l->loop)) fatal("uv_loop_close");
}

static void try_call_user_onterm(struct looper *l)
{
    if (l->closed.async && l->closed.sigterm && l->closed.sigint)
        (*l->onterm_cb)();
}

static void onclose_sigterm_cb(struct uv_handle_s *handle)
{
    struct looper *l = handle->data;
    l->closed.sigterm = true;
    try_call_user_onterm(l);
}

static void close_sigterm(struct looper *l)
{
    if (!l->closing.sigterm)
    {
        uv_close((struct uv_handle_s *)&l->sigterm, onclose_sigterm_cb);
        l->closing.sigterm = true;
    }
}

static void onclose_sigint_cb(struct uv_handle_s *handle)
{
    struct looper *l = handle->data;
    l->closed.sigint = true;
    try_call_user_onterm(l);
}

static void close_sigint(struct looper *l)
{
    if (!l->closing.sigint)
    {
        uv_close((struct uv_handle_s *)&l->sigint, onclose_sigint_cb);
        l->closing.sigint = true;
    }
}

static void onclose_async_cb(struct uv_handle_s *handle)
{
    struct looper *l = handle->data;
    l->closed.async = true;
    close_sigterm(l);
    close_sigint(l);
}

static void async_cb(struct uv_async_s *handle)
{
    struct looper *l = handle->data;
    l->terminating = true;
    if (!l->closing.async)
    {
        uv_close((struct uv_handle_s *)&l->async, onclose_async_cb);
        l->closing.async = true;
    }
}

static void sigterm_cb(struct uv_signal_s *handle, int signum)
{
    (void)signum;
    looper_terminate(handle->data);
}

static void sigint_cb(struct uv_signal_s *handle, int signum)
{
    (void)signum;
    looper_terminate(handle->data);
}

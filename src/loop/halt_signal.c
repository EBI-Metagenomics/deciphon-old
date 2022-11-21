#define _POSIX_C_SOURCE 200809L
#include "loop/halt_signal.h"
#include "die.h"
#include "unused.h"
#include <stdbool.h>
#include <uv.h>

static on_signal_fn_t *on_signal = NULL;

static struct uv_signal_s sigterm = {0};
static struct uv_signal_s sigint = {0};
static bool no_close = true;

static void fwd0(struct uv_signal_s *, int);
static void fwd1(struct uv_handle_s *);

void halt_signal_open(struct uv_loop_s *loop, on_signal_fn_t *on_signal_cb)
{
    on_signal = on_signal_cb;

    if (uv_signal_init(loop, &sigterm)) die();
    if (uv_signal_init(loop, &sigint)) die();

    if (uv_signal_start(&sigterm, &fwd0, SIGTERM)) die();
    if (uv_signal_start(&sigint, &fwd0, SIGINT)) die();

    no_close = false;
}

void halt_signal_close(void)
{
    if (no_close) return;
    uv_close((struct uv_handle_s *)&sigterm, NULL);
    uv_close((struct uv_handle_s *)&sigint, NULL);
    no_close = true;
}

static void fwd0(struct uv_signal_s *h, int s)
{
    unused(h);
    unused(s);
    if (no_close) return;
    uv_close((struct uv_handle_s *)&sigterm, &fwd1);
    uv_close((struct uv_handle_s *)&sigint, &fwd1);
    no_close = true;
}

static void fwd1(struct uv_handle_s *h)
{
    unused(h);
    static int remain = 2;
    if (!--remain) (*on_signal)();
}

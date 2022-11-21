#define _POSIX_C_SOURCE 200809L
#include "loop/linger.h"
#include "die.h"
#include "loop/global.h"
#include "loop/now.h"
#include "run_once.h"
#include "unused.h"
#include <stdbool.h>
#include <stddef.h>
#include <uv.h>

static struct uv_timer_s timer = {0};
static bool no_close = true;
static long deadline = 0;
static on_linger_fn_t *on_linger = NULL;
static on_cleanup_fn_t *on_cleanup = NULL;

void linger_init(struct uv_loop_s *loop, on_linger_fn_t *linger,
                 on_cleanup_fn_t *cleanup)
{
    if (uv_timer_init(loop, &timer)) die();
    no_close = false;
    deadline = 0;
    on_linger = linger;
    on_cleanup = cleanup;
}

static void linger(struct uv_timer_s *timer);

void linger_trigger(void)
{
    RUN_ONCE;
    deadline = now() + 5000L;
    if (uv_timer_start(&timer, &linger, 0, 10)) die();
}

static void cleanup_fwd(struct uv_handle_s *h);

void linger_cleanup(void)
{
    if (no_close) return;
    uv_close((struct uv_handle_s *)&timer, &cleanup_fwd);
    no_close = true;
}

static void linger(struct uv_timer_s *t)
{
    unused(t);
    if (now() < deadline)
    {
        if (on_linger && (*on_linger)()) deadline = 0;
    }
    else
        linger_cleanup();
}

static void cleanup_fwd(struct uv_handle_s *h)
{
    unused(h);
    if (on_cleanup) (*on_cleanup)();
}

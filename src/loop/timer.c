#include "loop/timer.h"
#include "core/global.h"
#include "core/logy.h"
#include "uv.h"
#include <stdatomic.h>
#include <stdbool.h>

inline static void set_disabled(struct timer *t, bool v)
{
    atomic_store_explicit(&t->disabled, v, memory_order_release);
}

inline static bool is_disabled(struct timer *t)
{
    return atomic_load_explicit(&t->disabled, memory_order_consume);
}

static void fwd_callback(struct uv_timer_s *req)
{
    struct timer *t = req->data;
    if (!is_disabled(t)) (*t->callback)();
}

void timer_init(struct timer *timer, long polling, timer_fn_t *fn)
{
    timer->polling = polling;
    timer->callback = fn;
    set_disabled(timer, timer->polling <= 0);

    if (uv_timer_init(global_loop(), &timer->uvtimer)) efail("uv_timer_init");
    timer->uvtimer.data = timer;

    if (!is_disabled(timer))
    {
        if (uv_timer_start(&timer->uvtimer, &fwd_callback, 0, polling))
            efail("uv_timer_start");
    }
}

void timer_cleanup(struct timer *timer)
{
    if (!is_disabled(timer)) uv_timer_stop(&timer->uvtimer);
}

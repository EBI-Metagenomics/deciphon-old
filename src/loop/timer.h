#ifndef LOOP_TIMER_H
#define LOOP_TIMER_H

#include <stdatomic.h>
#include <stdbool.h>
#include <uv.h>

typedef void timer_fn_t(void);

struct timer
{
    long polling;
    timer_fn_t *callback;
    atomic_bool disabled;
    struct uv_timer_s uvtimer;
};

void timer_init(struct timer *, long polling, timer_fn_t *);
void timer_cleanup(struct timer *);

#endif

#include "core/progress.h"
#include "core/compiler.h"
#include <stdio.h>

static int percent(long total, long consumed);

void progress_init(struct progress *progress, long total)
{
    progress->total = total;
    progress->consumed = 0;
}

int progress_consume(struct progress *p, long total)
{
    long prev = p->consumed;
    long next = prev + total;
    int inc = percent(p->total, next) - percent(p->total, prev);
    atomic_store_explicit(&p->consumed, next, memory_order_release);
    return inc;
}

unsigned progress_percent(struct progress *p)
{
    long consumed = atomic_load_explicit(&p->consumed, memory_order_consume);
    return (unsigned)percent(p->total, consumed);
}

static int percent(long total, long consumed)
{
    return (int)((consumed * 100L) / total);
}

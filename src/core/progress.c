#include "core/progress.h"
#include "core/compiler.h"
#include <stdio.h>

static int percent(long total, long consumed);

void progress_init(struct progress *progress, long total)
{
    progress->total = total;
    atomic_store(&progress->consumed, 0);
}

int progress_consume(struct progress *p, long total)
{
    long prev = atomic_load(&p->consumed);
    long next = prev + total;
    int inc = percent(p->total, next) - percent(p->total, prev);
    atomic_store(&p->consumed, next);
    return inc;
}

unsigned progress_percent(struct progress *p)
{
    return (unsigned)percent(p->total, atomic_load(&p->consumed));
}

static int percent(long total, long consumed)
{
    return (int)((consumed * 100L) / total);
}

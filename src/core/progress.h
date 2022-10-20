#ifndef CORE_PROGRESS_H
#define CORE_PROGRESS_H

#include <stdatomic.h>

struct progress
{
    long total;
    atomic_long consumed;
};

void progress_init(struct progress *, long total);
int progress_consume(struct progress *, long total);
long progress_total(struct progress const *);
long progress_consumed(struct progress const *);
int progress_percent(struct progress const *);

#endif

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
unsigned progress_percent(struct progress *);

#endif

#ifndef WORKER_H
#define WORKER_H

#include "lib/c11threads.h"
#include <stdbool.h>

struct dcp_partition;

struct worker
{
    struct dcp_partition* partition;
    thrd_t                thread;
    _Atomic(bool) finished;
};

struct worker* worker_create(struct dcp_partition* part);
void           worker_destroy(struct worker* worker);

#endif

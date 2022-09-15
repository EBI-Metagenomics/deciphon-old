#ifndef CORE_MEMPOOL_H
#define CORE_MEMPOOL_H

#include <stddef.h>

struct mempool;

struct mempool *mempool_new(unsigned bits, size_t object_size);
void mempool_del(struct mempool *);

void *mempool_new_object(struct mempool *);
void mempool_del_object(struct mempool *, void *object);

#endif

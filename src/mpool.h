#ifndef MPOOL_H
#define MPOOL_H

struct mpool;

void*         mpool_alloc(struct mpool* pool);
struct mpool* mpool_create(unsigned slot_size, unsigned power_size);
void          mpool_destroy(struct mpool const* pool);
void          mpool_free(struct mpool* pool, void const* slot);
unsigned      mpool_nslots(struct mpool* pool);
void*         mpool_slot(struct mpool* pool, unsigned i);
void*         mpool_tryalloc(struct mpool* pool);

#endif

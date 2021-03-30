#ifndef MPOOL_H
#define MPOOL_H

struct mpool;

typedef void (*mpool_deinit_slot_cb)(void* slot);
typedef void (*mpool_init_slot_cb)(void* slot);

void*         mpool_alloc(struct mpool* pool);
struct mpool* mpool_create(unsigned slot_size, unsigned power_size, mpool_init_slot_cb init);
void          mpool_destroy(struct mpool const* pool, mpool_deinit_slot_cb deinit);
void          mpool_free(struct mpool* pool, void const* slot);

#endif

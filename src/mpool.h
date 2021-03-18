#ifndef MPOOL_H
#define MPOOL_H

#include "llist.h"
#include <stddef.h>
#include <stdlib.h>

struct mpool
{
    size_t               slot_size;
    unsigned char const* memory;
    struct llist_list    list;
};

#define MPOOL_INIT(pool, type, nslots, member) mpool_init(pool, sizeof(type), nslots, offsetof(type, member));

static inline void mpool_init(struct mpool* pool, size_t slot_size, unsigned nslots, size_t node_offset)
{
    unsigned char* memory = malloc(slot_size * nslots);
    llist_init_list(&pool->list);

    for (unsigned i = 0; i < nslots; ++i) {
        unsigned char*     slot = memory + slot_size * i;
        struct llist_node* node = (struct llist_node*)(slot + node_offset);
        llist_add(&pool->list, node);
    }
    pool->slot_size = slot_size;
    pool->memory = memory;
}

static inline struct llist_node* mpool_alloc(struct mpool* pool) { return llist_pop(&pool->list); }

static inline void mpool_free(struct mpool* pool, struct llist_node* node) { llist_add(&pool->list, node); }

static inline void mpool_deinit(struct mpool const* pool) { free((void*)pool->memory); }

static inline void* mpool_slot(struct mpool* pool, unsigned i) { return (void*)(pool->memory + i * pool->slot_size); }

#endif

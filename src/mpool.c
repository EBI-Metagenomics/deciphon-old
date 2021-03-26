#include "mpool.h"
#include "util.h"
#include <ck_ring.h>

struct mpool
{
    unsigned       slot_size;
    unsigned       nslots;
    unsigned char* memory;

    ck_ring_t ring    CK_CC_CACHELINE;
    ck_ring_buffer_t* buffer;
};

void* mpool_alloc(struct mpool* pool)
{
    unsigned char* slot = NULL;
    return ck_ring_dequeue_mpmc(&pool->ring, pool->buffer, &slot) ? slot : NULL;
}

struct mpool* mpool_create(unsigned slot_size, unsigned power_size)
{
    BUG(power_size < 4);
    unsigned buffsize = 1 << power_size;

    struct mpool* pool = malloc(sizeof(*pool));
    pool->slot_size = slot_size;
    pool->nslots = buffsize - 1;
    pool->memory = malloc(slot_size * pool->nslots);

    ck_ring_init(&pool->ring, buffsize);
    pool->buffer = calloc(buffsize, sizeof(*pool->buffer));

    unsigned char* slot = pool->memory;
    for (unsigned i = 0; i < pool->nslots; ++i) {
        BUG(!ck_ring_enqueue_mpmc(&pool->ring, pool->buffer, slot));
        slot += slot_size;
    }
    return pool;
}

void mpool_destroy(struct mpool const* pool)
{
    BUG(ck_ring_size(&pool->ring) != pool->nslots);
    free(pool->memory);
    free(pool->buffer);
    free((void*)pool);
}

void mpool_free(struct mpool* pool, void const* slot) { BUG(!ck_ring_enqueue_mpmc(&pool->ring, pool->buffer, slot)); }

unsigned mpool_nslots(struct mpool* pool) { return pool->nslots; }

void* mpool_slot(struct mpool* pool, unsigned i) { return (void*)(pool->memory + i * pool->slot_size); }
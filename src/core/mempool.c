/* Acknowledgment: https://github.com/dcreager/libcork */
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "core/mempool.h"

struct proxy
{
    struct proxy *next_free;
};

struct block
{
    struct block *next_block;
};

struct mempool
{
    size_t object_size;
    size_t block_size;
    struct proxy *free_list;
    size_t allocated_count;
    struct block *blocks;
};

/* chunk is the memory allocated to fit both proxy and its object */
static inline size_t object_chunk_size(struct mempool const *mp)
{
    return (sizeof(struct proxy) + (mp)->object_size);
}

static inline struct proxy *get_proxy_from_object(void *object)
{
    return (struct proxy *)(object)-1;
}

static inline void *get_object_from_proxy(struct proxy *proxy)
{
    return ((struct proxy *)(proxy)) + 1;
}

struct mempool *mempool_new(unsigned bits, size_t object_size)
{
    struct mempool *mp = malloc(sizeof(struct mempool));
    if (!mp) return 0;
    mp->object_size = object_size;
    mp->block_size = 1 << (bits);
    mp->free_list = 0;
    mp->allocated_count = 0;
    mp->blocks = 0;
    assert(object_chunk_size(mp) <= mp->block_size);
    return mp;
}

void mempool_del(struct mempool *mp)
{
    assert(mp->allocated_count == 0);

    struct block *curr = mp->blocks;
    while (curr)
    {
        struct block *next = curr->next_block;
        free(curr);
        curr = next;
    }

    free(mp);
}

static void divide_block_into_chunks(struct mempool *mp, struct block *block)
{
    void *offset = block;
    size_t pos = sizeof(struct block);
    while (pos + object_chunk_size(mp) <= mp->block_size)
    {
        struct proxy *proxy = offset + pos;
        proxy->next_free = mp->free_list;
        mp->free_list = proxy;
        pos += object_chunk_size(mp);
    }
}

static bool new_block(struct mempool *mp)
{
    struct block *block = malloc(mp->block_size);
    if (!block) return false;
    block->next_block = mp->blocks;
    mp->blocks = block;

    divide_block_into_chunks(mp, block);
    return true;
}

void *mempool_new_object(struct mempool *mp)
{
    if (!mp->free_list && !new_block(mp)) return 0;

    struct proxy *obj = mp->free_list;
    mp->free_list = obj ? obj->next_free : 0;
    mp->allocated_count++;
    return get_object_from_proxy(obj);
}

void mempool_del_object(struct mempool *mp, void *object)
{
    struct proxy *proxy = get_proxy_from_object(object);
    proxy->next_free = mp->free_list;
    mp->free_list = proxy;
    mp->allocated_count--;
}

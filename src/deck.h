#ifndef deck_H
#define deck_H

#include "bitops.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct dnode
{
    unsigned idx;
};

struct deck
{
    uint32_t avail;
    struct dnode *nodes[32];
};

static inline void deck_init(struct deck *deck)
{
    deck->avail = 0;
    for (unsigned i = 0; i < 32; ++i)
        deck->nodes[i] = NULL;
}

static inline void deck_assoc(struct deck *deck, struct dnode *node)
{
    int i = bitops_fful(deck->avail ^ UINT32_MAX);
    BITOPS_SET(deck->avail, i);
    node->idx = (unsigned)i;
    deck->nodes[i] = node;
}

static inline struct dnode *deck_pop(struct deck *deck)
{
    int i = bitops_fful(deck->avail);
    BITOPS_CLR(deck->avail, i);
    return deck->nodes[i];
}

static inline void deck_put(struct deck *deck, struct dnode *node)
{
    BITOPS_SET(deck->avail, node->idx);
}

static inline bool deck_full(struct deck *deck)
{
    return deck->avail == UINT32_MAX;
}

static inline bool deck_empty(struct deck *deck) { return deck->avail == 0; }

#endif

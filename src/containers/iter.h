#ifndef CONTAINERS_ITER_H
#define CONTAINERS_ITER_H

#include "containers/container.h"
#include "containers/dnode.h"
#include "containers/snode.h"

struct iter_dnode
{
    struct dnode*       curr;
    struct dnode const* end;
};

struct iter_snode
{
    struct snode*       curr;
    struct snode const* end;
};

static inline struct dnode* iter_dnode_next(struct iter_dnode* iter);

static inline struct dnode* iter_dnode_next(struct iter_dnode* iter)
{
    if (iter->curr == iter->end)
        return NULL;
    struct dnode* node = iter->curr;
    iter->curr = node->next;
    return node;
}

static inline struct snode* iter_snode_next(struct iter_snode* iter);

static inline struct snode* iter_snode_next(struct iter_snode* iter)
{
    if (iter->curr == iter->end)
        return NULL;
    struct snode* node = iter->curr;
    iter->curr = node->next;
    return node;
}

#define ITER_NEXT(X) _Generic((X), struct iter_snode * : iter_snode_next, struct iter_dnode * : iter_dnode_next)(X)

#define ITER_FOREACH(entry, iter, member)                                                                              \
    for (entry = CONTAINER_OF_OR_NULL(ITER_NEXT(iter), __typeof__(*entry), member); entry;                             \
         entry = CONTAINER_OF_OR_NULL(ITER_NEXT(iter), __typeof__(*entry), member))

#endif

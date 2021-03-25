#ifndef CONTAINERS_SNODE_H
#define CONTAINERS_SNODE_H

#include <stddef.h>

struct snode
{
    struct snode* next;
};

#define SNODE_INIT()                                                                                                   \
    {                                                                                                                  \
        NULL                                                                                                           \
    }

static inline void snode_add_next(struct snode* where, struct snode* new);
static inline void snode_deinit(struct snode* node);
static inline void snode_del(struct snode* prev, struct snode* node);
static inline void snode_init(struct snode* node);

static inline void snode_add_next(struct snode* where, struct snode* new)
{
    new->next = where->next;
    where->next = new;
}

static inline void snode_deinit(struct snode* node) { node->next = NULL; }

static inline void snode_del(struct snode* prev, struct snode* node)
{
    prev->next = node->next;
    node->next = NULL;
}

static inline void snode_init(struct snode* node) { node->next = NULL; }

#endif

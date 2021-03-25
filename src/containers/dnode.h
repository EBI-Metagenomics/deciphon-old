#ifndef CONTAINERS_DNODE_H
#define CONTAINERS_DNODE_H

#include <stdbool.h>
#include <stddef.h>

struct dnode
{
    struct dnode* next;
    struct dnode* prev;
};

#define NODE_INIT(name)                                                                                                \
    {                                                                                                                  \
        &(name), &(name)                                                                                               \
    }

static inline void dnode_add_between(struct dnode* prev, struct dnode* next, struct dnode* new);
static inline void dnode_add_next(struct dnode* where, struct dnode* new);
static inline void dnode_add_prev(struct dnode* where, struct dnode* new);
static inline void dnode_deinit(struct dnode* node);
static inline void dnode_del(struct dnode* node);
static inline void dnode_init(struct dnode* node);

static inline void dnode_add_between(struct dnode* prev, struct dnode* next, struct dnode* new)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void dnode_add_next(struct dnode* where, struct dnode* new)
{
    dnode_add_between(where, where->next, new);
}

static inline void dnode_add_prev(struct dnode* where, struct dnode* new)
{
    dnode_add_between(where->prev, where, new);
}

static inline void dnode_deinit(struct dnode* node)
{
    node->prev = NULL;
    node->next = NULL;
}

static inline void dnode_del(struct dnode* node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

static inline void dnode_init(struct dnode* node)
{
    node->next = node;
    node->prev = node;
}

#endif

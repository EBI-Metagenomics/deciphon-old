#ifndef NODE_H
#define NODE_H

#include <stdbool.h>
#include <stddef.h>

struct node
{
    struct node* next;
    struct node* prev;
};

#define NODE_INIT(name)                                                                                                \
    {                                                                                                                  \
        &(name), &(name)                                                                                               \
    }

static inline void         node_add_between(struct node* prev, struct node* next, struct node* new);
static inline void         node_add_next(struct node* where, struct node* new);
static inline void         node_add_prev(struct node* where, struct node* new);
static inline void         node_deinit(struct node* node);
static inline void         node_del(struct node* node);
static inline void         node_init(struct node* node);
static inline struct node* node_next(struct node* node);
static inline struct node* node_prev(struct node* node);
static inline bool         node_single(struct node const* node);

static inline void node_add_between(struct node* prev, struct node* next, struct node* new)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void node_add_next(struct node* where, struct node* new) { node_add_between(where, where->next, new); }

static inline void node_add_prev(struct node* where, struct node* new) { node_add_between(where->prev, where, new); }

static inline void node_deinit(struct node* node)
{
    node->prev = NULL;
    node->next = NULL;
}

static inline void node_del(struct node* node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

static inline void node_init(struct node* node)
{
    node->next = node;
    node->prev = node;
}

static inline struct node* node_next(struct node* node) { return node->next; }

static inline struct node* node_prev(struct node* node) { return node->prev; }

static inline bool node_single(struct node const* node) { return node == node->next; }

#endif

#ifndef LLIST_H
#define LLIST_H

#include "container.h"

struct llist_node
{
    struct llist_node* next;
};

struct llist_list
{
    struct llist_node* first;
};

static inline void llist_init_list(struct llist_list* list) { list->first = NULL; }

static inline void llist_init_node(struct llist_node* node) { node->next = NULL; }

static inline struct llist_node* llist_head(struct llist_list* list) { return list->first; }

static inline struct llist_node* llist_next(struct llist_node* node) { return node->next; }

static inline void llist_add(struct llist_list* list, struct llist_node* node)
{
    node->next = list->first;
    list->first = node;
}

static inline void llist_del(struct llist_list* list) { list->first = list->first ? list->first->next : NULL; }

#endif

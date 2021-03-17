#ifndef LIST_H
#define LIST_H

#include "container.h"

struct list
{
    struct list* next;
    struct list* prev;
};

static inline void list_init(struct list* list)
{
    list->next = list;
    list->prev = list;
}

static inline struct list* list_head(struct list const* list) { return list->next == list ? NULL : list->next; }

static inline struct list* list_tail(struct list const* list) { return list->next == list ? NULL : list->prev; }

static inline struct list* list_next(struct list const* list, struct list const* node)
{
    return node->next == list ? NULL : node->next;
}

static inline void list_ins(struct list* where, struct list* node)
{
    struct list* prev = where->prev;
    struct list* next = where;

    next->prev = node;
    node->next = next;
    node->prev = prev;
    prev->next = node;
}

static inline void list_add(struct list* list, struct list* node)
{
    struct list* next = list;
    struct list* prev = list->prev;

    next->prev = node;
    node->next = next;
    node->prev = prev;
    prev->next = node;
}

static inline void list_del(struct list* node)
{
    struct list* next = node->next;
    struct list* prev = node->prev;

    prev->next = next;
    next->prev = prev;

    /*
     * These are non-NULL pointers that will result in page faults
     * under normal circumstances, used to verify that nobody uses
     * non-initialized list entries. Reference: Linux kernel.
     */
    node->next = (void*)0x100;
    node->prev = (void*)0x122;
}

#endif

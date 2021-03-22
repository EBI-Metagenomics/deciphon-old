#ifndef LIST_H
#define LIST_H

#include "container.h"

struct list_head
{
    struct list_head* next;
    struct list_head* prev;
};

static inline void list_init(struct list_head* head)
{
    head->next = head;
    head->prev = head;
}

static inline struct list_head* list_first(struct list_head const* head)
{
    return head->next == head ? NULL : head->next;
}

static inline struct list_head* list_last(struct list_head const* head)
{
    return head->next == head ? NULL : head->prev;
}

static inline struct list_head* list_next(struct list_head const* head, struct list_head const* node)
{
    return node->next == head ? NULL : node->next;
}

static inline void list_ins(struct list_head* where, struct list_head* node)
{
    struct list_head* prev = where->prev;
    struct list_head* next = where;

    next->prev = node;
    node->next = next;
    node->prev = prev;
    prev->next = node;
}

static inline void list_add(struct list_head* head, struct list_head* node)
{
    struct list_head* next = head;
    struct list_head* prev = head->prev;

    next->prev = node;
    node->next = next;
    node->prev = prev;
    prev->next = node;
}

static inline void list_del(struct list_head* node)
{
    struct list_head* next = node->next;
    struct list_head* prev = node->prev;

    prev->next = next;
    next->prev = prev;

    /*
     * These are non-NULL pointers that will result in page faults
     * under normal circumstances, used to verify that nobody uses
     * non-initialized head entries. Reference: Linux kernel.
     */
    node->next = (void*)0x100;
    node->prev = (void*)0x122;
}

#endif

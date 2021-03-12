#ifndef LIST_H
#define LIST_H

#include <stddef.h>

#define container_of(ptr, type, member) ((type*)((size_t)(void*)(ptr)-offsetof(type, member)))

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

static inline struct list* list_next(struct list const* list, struct list const* item)
{
    return item->next == list ? NULL : item->next;
}

static inline void list_ins(struct list* where, struct list* item)
{
    struct list* prev = where->prev;
    struct list* next = where;

    next->prev = item;
    item->next = next;
    item->prev = prev;
    prev->next = item;
}

static inline void list_add(struct list* list, struct list* item)
{
    struct list* next = list;
    struct list* prev = list->prev;

    next->prev = item;
    item->next = next;
    item->prev = prev;
    prev->next = item;
}

static inline void list_del(struct list* item)
{
    struct list* next = item->next;
    struct list* prev = item->prev;

    prev->next = next;
    next->prev = prev;

    /*
     * These are non-NULL pointers that will result in page faults
     * under normal circumstances, used to verify that nobody uses
     * non-initialized list entries. Reference: Linux kernel.
     */
    item->next = (void*)0x100;
    item->prev = (void*)0x122;
}

#endif

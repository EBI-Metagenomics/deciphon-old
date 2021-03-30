#ifndef CONTAINERS_QUEUE_H
#define CONTAINERS_QUEUE_H

#include "containers/iter.h"
#include "containers/snode.h"
#include <stddef.h>

struct queue
{
    struct snode  head;
    struct snode* tail;
};

#define QUEUE_INIT()                                                                                                   \
    {                                                                                                                  \
        SNODE_INIT(), NULL                                                                                             \
    }

static inline bool          queue_empty(struct queue const* queue);
static inline void          queue_init(struct queue* queue);
static inline struct snode* queue_pop(struct queue* queue);
static inline void          queue_push(struct queue* queue, struct snode* new);

static inline bool queue_empty(struct queue const* queue) { return queue->head.next == NULL; }

static inline void queue_init(struct queue* queue)
{
    snode_init(&queue->head);
    queue->tail = NULL;
}

static inline struct snode* queue_pop(struct queue* queue)
{
    struct snode* node = queue->head.next;
    snode_del(&queue->head, node);
    return node;
}

static inline void queue_push(struct queue* queue, struct snode* new)
{
    if (queue_empty(queue))
        queue->tail = new;

    snode_add_next(&queue->head, new);
}

#endif

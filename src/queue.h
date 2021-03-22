#ifndef QUEUE_H
#define QUEUE_H

#include "node.h"
#include <stddef.h>

struct queue
{
    struct node head;
};

#define QUEUE_INIT(name)                                                                                               \
    {                                                                                                                  \
        NODE_INIT((name).head)                                                                                         \
    }

static inline void         queue_deinit(struct queue* queue);
static inline bool         queue_empty(struct queue const* queue);
static inline void         queue_init(struct queue* queue);
static inline struct node* queue_pop(struct queue* queue);
static inline void         queue_push(struct queue* queue, struct node* new);

static inline void queue_deinit(struct queue* queue) { node_deinit(&queue->head); }

static inline bool queue_empty(struct queue const* queue) { return node_single(&queue->head); }

static inline void queue_init(struct queue* queue) { node_init(&queue->head); }

static inline struct node* queue_pop(struct queue* queue)
{
    if (queue_empty(queue))
        return NULL;

    struct node* node = node_prev(&queue->head);
    node_del(node);
    return node;
}

static inline void queue_push(struct queue* queue, struct node* new) { node_add_next(&queue->head, new); }

#endif

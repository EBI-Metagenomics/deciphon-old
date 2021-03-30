#include "fifo.h"
#include "dthread.h"
#include <stdlib.h>

struct fifo
{
    pthread_mutex_t mutex;
    struct queue    queue;
    bool            closed;
};

static inline void lock(struct fifo* fifo) { dthread_lock(&fifo->mutex); }
static inline void unlock(struct fifo* fifo) { dthread_unlock(&fifo->mutex); }

void fifo_close(struct fifo* fifo)
{
    lock(fifo);
    fifo->closed = true;
    unlock(fifo);
}

bool fifo_closed(struct fifo* fifo)
{
    lock(fifo);
    bool closed = fifo->closed;
    unlock(fifo);

    return closed;
}

struct fifo* fifo_create(void)
{
    struct fifo* fifo = malloc(sizeof(*fifo));
    dthread_mutex_init(&fifo->mutex);
    queue_init(&fifo->queue);
    fifo->closed = false;
    return fifo;
}

void fifo_destroy(struct fifo* fifo)
{
    dthread_mutex_destroy(&fifo->mutex);
    free(fifo);
}

bool fifo_empty(struct fifo* fifo)
{
    lock(fifo);
    bool empty = queue_empty(&fifo->queue);
    unlock(fifo);

    return empty;
}

struct snode* fifo_pop(struct fifo* fifo)
{
    lock(fifo);
    struct snode* node = !queue_empty(&fifo->queue) ? queue_pop(&fifo->queue) : NULL;
    unlock(fifo);

    return node;
}

void fifo_push(struct fifo* fifo, struct snode* new)
{
    lock(fifo);
    queue_push(&fifo->queue, new);
    unlock(fifo);
}

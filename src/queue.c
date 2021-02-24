#include "queue.h"
#include "bug.h"
#include "lib/c-list.h"
#include "mythreads.h"
#include "task.h"
#include <stdbool.h>

struct queue
{
    struct CList tasks;
    uint32_t     ntasks;
    uint32_t     max_size;

    mtx_t mtx;
    cnd_t empty;
    cnd_t full;
    bool  finished;
};

struct queue* queue_create(uint32_t max_size)
{
    struct queue* queue = malloc(sizeof(*queue));

    c_list_init(&queue->tasks);
    queue->ntasks = 0;
    queue->max_size = max_size;

    mtx_init(&queue->mtx, mtx_plain);
    cnd_init(&queue->empty);
    cnd_init(&queue->full);
    queue->finished = false;

    return queue;
}

void queue_destroy(struct queue* queue)
{
    BUG(queue->ntasks > 0);
    mtx_destroy(&queue->mtx);
    cnd_destroy(&queue->empty);
    cnd_destroy(&queue->full);
    free(queue);
}

void queue_finish(struct queue* queue)
{
    mtx_lock(&queue->mtx);
    queue->finished = true;
    mtx_unlock(&queue->mtx);
}

struct task* queue_pop(struct queue* queue)
{
    mtx_lock(&queue->mtx);

    while (queue->ntasks == 0 && !queue->finished) {
        cnd_wait(&queue->empty, &queue->mtx);
    }

    if (queue->ntasks == 0 && queue->finished) {
        mtx_unlock(&queue->mtx);
        return NULL;
    }

    struct CList* elem = c_list_first(&queue->tasks);
    struct task*  task = NULL;
    if (elem) {
        task = c_list_entry(elem, struct task, link);
        c_list_unlink(elem);
        queue->ntasks--;
        if (queue->ntasks + 1 == queue->max_size)
            cnd_broadcast(&queue->full);
    }

    mtx_unlock(&queue->mtx);
    return task;
}

void queue_push(struct queue* queue, struct task* task)
{
    mtx_lock(&queue->mtx);

    while (queue->ntasks >= queue->max_size) {
        struct timespec now;
        timespec_get(&now, TIME_UTC);
        now.tv_sec += 1;
        cnd_timedwait(&queue->full, &queue->mtx, &now);
    }

    c_list_link_tail(&queue->tasks, &task->link);
    queue->ntasks++;

    if (queue->ntasks == 1)
        cnd_broadcast(&queue->empty);

    mtx_unlock(&queue->mtx);
}

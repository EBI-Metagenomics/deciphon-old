#include "task_queue.h"
#include "task.h"
#include "util.h"

static void lock(struct task_queue* taskq);
static void unlock(struct task_queue* taskq);

void task_queue_deinit(struct task_queue* taskq)
{
    BUG(pthread_mutex_destroy(&taskq->mut));
    queue_deinit(&taskq->queue);
}

bool task_queue_empty(struct task_queue* taskq)
{
    lock(taskq);
    bool empty = queue_empty(&taskq->queue);
    unlock(taskq);

    return empty;
}

void task_queue_init(struct task_queue* taskq)
{
    BUG(pthread_mutex_init(&taskq->mut, NULL));
    queue_init(&taskq->queue);
}

struct dcp_task* task_queue_pop(struct task_queue* taskq)
{
    lock(taskq);
    struct snode*    node = !queue_empty(&taskq->queue) ? queue_pop(&taskq->queue) : NULL;
    struct dcp_task* task = CONTAINER_OF_OR_NULL(node, struct dcp_task, node);
    unlock(taskq);

    return task;
}

void task_queue_push(struct task_queue* taskq, struct dcp_task* task)
{
    lock(taskq);
    queue_push(&taskq->queue, &task->node);
    unlock(taskq);
}

static void lock(struct task_queue* taskq) { BUG(pthread_mutex_lock(&taskq->mut)); }

static void unlock(struct task_queue* taskq) { BUG(pthread_mutex_unlock(&taskq->mut)); }

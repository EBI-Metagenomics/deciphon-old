#include "container.h"
#include "queue.h"
#include "task.h"
#include "util.h"
#include <pthread.h>

struct task_queue
{
    pthread_mutex_t mut;
    struct queue    tasks;
};

void             task_queue_init(struct task_queue* taskq);
void             task_queue_deinit(struct task_queue* taskq);
struct dcp_task* task_queue_pop(struct task_queue* taskq);
void             task_queue_push(struct task_queue* taskq, struct dcp_task* task);

void task_queue_init(struct task_queue* taskq)
{
    if (pthread_mutex_init(&taskq->mut, NULL))
        die("mutex_init error");

    queue_init(&taskq->tasks);
}

void task_queue_deinit(struct task_queue* taskq)
{
    if (pthread_mutex_destroy(&taskq->mut))
        die("mutex_destroy error");

    queue_deinit(&taskq->tasks);
}

struct dcp_task* task_queue_pop(struct task_queue* taskq)
{
    if (pthread_mutex_lock(&taskq->mut))
        die("mutex_lock error");

    struct node*     node = queue_empty(&taskq->tasks) ? queue_pop(&taskq->tasks) : NULL;
    struct dcp_task* task = node ? CONTAINER_OF(node, struct dcp_task, node) : NULL;

    if (pthread_mutex_unlock(&taskq->mut))
        die("mutex_unlock error");

    return task;
}

void task_queue_push(struct task_queue* taskq, struct dcp_task* task)
{
    if (pthread_mutex_lock(&taskq->mut))
        die("mutex_lock error");

    queue_push(&taskq->tasks, &task->node);

    if (pthread_mutex_unlock(&taskq->mut))
        die("mutex_unlock error");
}

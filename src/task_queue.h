#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include "containers/queue.h"
#include <pthread.h>

struct task_queue
{
    pthread_mutex_t mut;
    struct queue    queue;
};

void             task_queue_deinit(struct task_queue* taskq);
bool             task_queue_empty(struct task_queue* taskq);
void             task_queue_init(struct task_queue* taskq);
struct dcp_task* task_queue_pop(struct task_queue* taskq);
void             task_queue_push(struct task_queue* taskq, struct dcp_task* task);

#endif

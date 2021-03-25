#ifndef RESULTS_QUEUE_H
#define RESULTS_QUEUE_H

#include "containers/queue.h"
#include <pthread.h>
#include <stdbool.h>

struct results_queue
{
    pthread_mutex_t mut;
    struct queue    queue;
};

void                results_queue_deinit(struct results_queue* queue);
bool                results_queue_empty(struct results_queue* queue);
void                results_queue_init(struct results_queue* queue);
struct dcp_results* results_queue_pop(struct results_queue* queue);
void                results_queue_push(struct results_queue* queue, struct dcp_results* results);

#endif

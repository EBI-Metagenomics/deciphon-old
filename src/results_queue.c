#include "results_queue.h"
#include "results.h"
#include "util.h"

static void lock(struct results_queue* queue);
static void unlock(struct results_queue* queue);

void results_queue_deinit(struct results_queue* queue)
{
    BUG(pthread_mutex_destroy(&queue->mut));
    queue_deinit(&queue->queue);
}

bool results_queue_empty(struct results_queue* queue)
{
    lock(queue);
    bool empty = queue_empty(&queue->queue);
    unlock(queue);

    return empty;
}

void results_queue_init(struct results_queue* queue)
{
    BUG(pthread_mutex_init(&queue->mut, NULL));
    queue_init(&queue->queue);
}

struct dcp_results* results_queue_pop(struct results_queue* queue)
{
    lock(queue);
    struct snode*       node = !queue_empty(&queue->queue) ? queue_pop(&queue->queue) : NULL;
    struct dcp_results* results = CONTAINER_OF_OR_NULL(node, struct dcp_results, node);
    unlock(queue);

    return results;
}

void results_queue_push(struct results_queue* queue, struct dcp_results* results)
{
    lock(queue);
    queue_push(&queue->queue, &results->node);
    unlock(queue);
}

static void lock(struct results_queue* queue) { BUG(pthread_mutex_lock(&queue->mut)); }

static void unlock(struct results_queue* queue) { BUG(pthread_mutex_unlock(&queue->mut)); }

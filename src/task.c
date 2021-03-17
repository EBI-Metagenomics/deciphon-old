#include "task.h"
#include "dcp/dcp.h"
#include "mpool.h"
#include "results.h"
#include "sequence.h"
#include <stdlib.h>
#include <string.h>

struct dcp_task
{
    struct dcp_task_cfg cfg;
    struct list         sequences;
    struct list         results;
    bool                finished;
    struct mpool        pool;
};

static void free_sequences(struct dcp_task* task);

void dcp_task_add_seq(struct dcp_task* task, char const* sequence)
{
    struct sequence* seq = malloc(sizeof(*seq));
    seq->sequence = strdup(sequence);
    list_init(&seq->link);
    list_add(&task->sequences, &seq->link);
}

struct dcp_task* dcp_task_create(struct dcp_task_cfg cfg)
{
    struct dcp_task* task = malloc(sizeof(*task));
    task->cfg = cfg;
    list_init(&task->sequences);
    list_init(&task->results);
    task->finished = false;
    MPOOL_INIT(&task->pool, struct dcp_results, 2, node);
    for (unsigned i = 0; i < 2; ++i)
        results_init(mpool_slot(&task->pool, i));
    return task;
}

void dcp_task_destroy(struct dcp_task const* task)
{
    free_sequences((struct dcp_task*)task);
    mpool_deinit(&task->pool);
    free((void*)task);
}

struct dcp_results* dcp_task_fetch_results(struct dcp_task* task)
{
    struct dcp_results* results = NULL;
#pragma omp critical
    {
        struct list* node = list_head(&task->results);
        if (node) {
            list_del(node);
            results = CONTAINER_OF(node, struct dcp_results, node);
        }
    }
    return results;
}

bool dcp_task_finished(struct dcp_task const* task)
{
    bool finished = false;
#pragma omp atomic read
    finished = task->finished;
    return finished;
}

void dcp_task_release_results(struct dcp_task* task, struct dcp_results* results)
{
#pragma omp critical
    mpool_free(&task->pool, &results->node);
}

/* void dcp_task_reset(struct dcp_task* task) */
/* { */
/*     free_sequences(task); */
/*     list_init(&task->results); */
/*     task->finished = false; */
/* } */

struct dcp_results* task_alloc_results(struct dcp_task* task)
{
    struct dcp_results* results = NULL;
#pragma omp critical
    {
        struct llist_node* node = mpool_alloc(&task->pool);
        if (node) {
            results = CONTAINER_OF(node, struct dcp_results, node);
            results_rewind(results);
        }
    }
    return results;
}

struct dcp_task_cfg const* task_cfg(struct dcp_task* task) { return &task->cfg; }

void task_finish(struct dcp_task* task)
{
#pragma omp atomic write
    task->finished = true;
}

struct sequence const* task_first_seq(struct dcp_task* task)
{
    struct list* i = list_head(&task->sequences);
    return i ? CONTAINER_OF(i, struct sequence, link) : NULL;
}

struct sequence const* task_next_seq(struct dcp_task* task, struct sequence const* sequence)
{
    struct list* i = list_next(&task->sequences, &sequence->link);
    return i ? CONTAINER_OF(i, struct sequence, link) : NULL;
}

void task_push_results(struct dcp_task* task, struct dcp_results* results)
{
#pragma omp critical
    list_add(&task->results, &results->link);
}

static void free_sequences(struct dcp_task* task)
{
    struct list* i = list_head(&task->sequences);
    while (i) {
        struct list tmp = *i;
        list_del(i);
        struct sequence* seq = CONTAINER_OF(i, struct sequence, link);
        free((void*)seq->sequence);
        free(seq);
        i = list_next(&task->sequences, &tmp);
    }
    list_init(&task->sequences);
}

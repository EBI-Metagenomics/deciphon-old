#include "task.h"
#include "ck_pr.h"
#include "dcp/dcp.h"
#include "mpool.h"
#include "results.h"
#include "sequence.h"
#include <stdlib.h>
#include <string.h>

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
    llist_init_list(&task->results);
    task->finished = 0;
    task->end = 0;
    MPOOL_INIT(&task->pool, struct dcp_results, 2, node);
    for (unsigned i = 0; i < 2; ++i)
        results_init(mpool_slot(&task->pool, i));
    llist_init_node(&task->link);
    return task;
}

void dcp_task_destroy(struct dcp_task const* task)
{
    free_sequences((struct dcp_task*)task);
    mpool_deinit(&task->pool);
    free((void*)task);
}

bool dcp_task_end(struct dcp_task const* task) { return ck_pr_load_int(&task->end); }

struct dcp_results* dcp_task_fetch_results(struct dcp_task* task)
{
    struct dcp_results* results = NULL;
    bool                finished = 0;

    while (!results && !finished) {
        struct llist_node* node = NULL;

#pragma omp critical
        node = llist_pop(&task->results);

        if (node)
            results = CONTAINER_OF(node, struct dcp_results, node);
        else
            ck_pr_stall();

#pragma omp critical
        finished = task->finished;
    }

#pragma omp critical
    task->end = task->finished && !llist_head(&task->results);

    return results;
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
/*     task->finished = 0; */
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

void task_finish(struct dcp_task* task) { ck_pr_store_int(&task->finished, 1); }

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
    printf("task_push_results\n");
    fflush(stdout);
#pragma omp critical
    llist_add(&task->results, &results->link);
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

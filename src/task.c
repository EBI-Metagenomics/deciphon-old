#include "task.h"
#include "dcp/dcp.h"
#include "results.h"
#include "seq.h"
#include "util.h"
#include <ck_pr.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

static void free_sequences(struct dcp_task* task);

void dcp_task_add_seq(struct dcp_task* task, char const* sequence)
{
    struct seq* seq = seq_create(sequence, task->seqid++);
    seq_stack_push(&task->sequences, seq);
}

struct dcp_task* dcp_task_create(struct dcp_task_cfg cfg)
{
    struct dcp_task* task = malloc(sizeof(*task));

    BUG(pthread_mutex_init(&task->mutex, NULL));
    BUG(pthread_cond_init(&task->cond, NULL));

    task->cfg = cfg;
    seq_stack_init(&task->sequences);
    task->seqid = 0;
    results_queue_init(&task->results);
    task->finished = 0;
    task->end = 0;
    task->status = TASK_STATUS_CREATED;
    snode_init(&task->node);

    return task;
}

void dcp_task_destroy(struct dcp_task* task)
{
    free_sequences(task);
    seq_stack_deinit(&task->sequences);
    results_queue_deinit(&task->results);
    snode_deinit(&task->node);
    free(task);
}

struct dcp_results* dcp_task_fetch_results(struct dcp_task* task) { return results_queue_pop(&task->results); }

int dcp_task_join(struct dcp_task* task)
{
    BUG(pthread_mutex_lock(&task->mutex));
    BUG(pthread_cond_wait(&task->cond, &task->mutex));
    return 0;
}

enum task_status dcp_task_status(struct dcp_task const* task)
{
    return (enum task_status)ck_pr_load_int(&task->status);
}

struct dcp_task_cfg const* task_cfg(struct dcp_task* task) { return &task->cfg; }

void task_finish(struct dcp_task* task, enum task_status status)
{
    ck_pr_store_int(&task->finished, status);
    BUG(pthread_cond_signal(&task->cond));
}

void task_push_results(struct dcp_task* task, struct dcp_results* results)
{
    results_queue_push(&task->results, results);
}

struct iter_snode task_seq_iter(struct dcp_task* task) { return seq_stack_iter(&task->sequences); }

static void free_sequences(struct dcp_task* task)
{
    struct seq* seq = NULL;
    goto enter;
    while (seq) {
        free(seq);
    enter:
        seq = seq_stack_pop(&task->sequences);
    }
}

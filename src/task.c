#include "task.h"
#include "dcp/dcp.h"
#include "seq.h"
#include "util.h"
#include <ck_pr.h>

void dcp_task_add_seq(struct dcp_task* task, char const* seq)
{
    seq_stack_push(&task->sequences, seq_create(seq, task->seqid++));
}

struct dcp_task* dcp_task_create(struct dcp_task_cfg cfg)
{
    struct dcp_task* task = malloc(sizeof(*task));

    task->cfg = cfg;
    seq_stack_init(&task->sequences);
    task->seqid = 0;
    results_queue_init(&task->results);
    task->end = 0;
    task->status = TASK_STATUS_CREATED;
    snode_init(&task->node);

    return task;
}

void dcp_task_destroy(struct dcp_task* task)
{
    struct seq* seq = NULL;
    goto enter;
    while (seq) {
        seq_destroy(seq);
    enter:
        seq = seq_stack_pop(&task->sequences);
    }

    seq_stack_deinit(&task->sequences);
    results_queue_deinit(&task->results);
    snode_deinit(&task->node);
    free(task);
}

bool dcp_task_end(struct dcp_task* task) { return ck_pr_load_int(&task->end); }

struct dcp_results* dcp_task_read(struct dcp_task* task)
{
    enum dcp_task_status status = dcp_task_status(task);
    if (status != TASK_STATUS_CREATED && results_queue_empty(&task->results))
        ck_pr_store_int(&task->end, 1);

    return results_queue_pop(&task->results);
}

enum dcp_task_status dcp_task_status(struct dcp_task const* task)
{
    return (enum dcp_task_status)ck_pr_load_int(&task->status);
}

void task_add_results(struct dcp_task* task, struct dcp_results* results)
{
    results_queue_push(&task->results, results);
}

struct dcp_task_cfg const* task_cfg(struct dcp_task* task) { return &task->cfg; }

struct iter_snode task_seq_iter(struct dcp_task* task) { return seq_stack_iter(&task->sequences); }

void task_set_status(struct dcp_task* task, enum dcp_task_status status) { ck_pr_store_int(&task->status, status); }

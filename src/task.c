#include "task.h"
#include "dcp/dcp.h"
#include "fifo.h"
#include "results.h"
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
    task->results = fifo_create();
    task->end = 0;
    task->errno = 0;
    snode_init(&task->bin_node);

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

    fifo_destroy(task->results);
    free(task);
}

bool dcp_task_end(struct dcp_task* task) { return ck_pr_load_int(&task->end); }

int dcp_task_errno(struct dcp_task const* task) { return ck_pr_load_int(&task->errno); }

struct dcp_results* dcp_task_read(struct dcp_task* task)
{
    if (fifo_closed(task->results) && fifo_empty(task->results))
        ck_pr_store_int(&task->end, 1);

    struct snode* node = fifo_pop(task->results);
    return CONTAINER_OF_OR_NULL(node, struct dcp_results, node);
}

void task_add_results(struct dcp_task* task, struct dcp_results* results) { fifo_push(task->results, &results->node); }

struct dcp_task_cfg const* task_cfg(struct dcp_task* task) { return &task->cfg; }

struct iter_snode task_seqiter(struct dcp_task* task) { return seq_stack_iter(&task->sequences); }

void task_seterr(struct dcp_task* task) { ck_pr_store_int(&task->errno, 1); }

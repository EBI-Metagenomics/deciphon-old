#ifndef TASK_H
#define TASK_H

#include "dcp/dcp.h"
#include "results_queue.h"
#include "seq_stack.h"

struct dcp_results;
struct dcp_task_cfg;
struct seq;

struct dcp_task
{
    struct dcp_task_cfg  cfg;
    struct seq_stack     sequences;
    uint32_t             seqid;
    struct results_queue results;
    int                  end;
    int                  status;
    struct snode         node;
};

void                       task_add_results(struct dcp_task* task, struct dcp_results* results);
struct dcp_task_cfg const* task_cfg(struct dcp_task* task);
struct iter_snode          task_seq_iter(struct dcp_task* task);
void                       task_set_status(struct dcp_task* task, enum dcp_task_status status);

#endif

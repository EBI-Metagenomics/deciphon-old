#ifndef TASK_H
#define TASK_H

#include "dcp/dcp.h"
#include "dcp/task.h"
#include "fifo1.h"
#include "seq_stack.h"

struct dcp_results;
struct dcp_task_cfg;
struct fifo;

struct dcp_task
{
    struct dcp_task_cfg cfg;
    struct seq_stack sequences;
    uint32_t seqid;
    struct fifo *results;
    int end;
    int errno;
    struct fifo1_node node;
    struct snode bin_node;
    struct dcp_results *curr_results;
};

void task_add_results(struct dcp_task *task, struct dcp_results *results);
struct dcp_task_cfg const *task_cfg(struct dcp_task *task);
void task_close_results(struct dcp_task *task);
void task_open_results(struct dcp_task *task);
struct iter_snode task_seqiter(struct dcp_task *task);
void task_seterr(struct dcp_task *task);

#endif

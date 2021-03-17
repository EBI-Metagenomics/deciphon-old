#ifndef TASK_H
#define TASK_H

struct dcp_results;
struct dcp_task;
struct dcp_task_cfg;
struct sequence;

struct dcp_results*        task_alloc_results(struct dcp_task* task);
struct dcp_task_cfg const* task_cfg(struct dcp_task* task);
void                       task_finish(struct dcp_task* task);
struct sequence const*     task_first_seq(struct dcp_task* task);
struct sequence const*     task_next_seq(struct dcp_task* task, struct sequence const* sequence);
void                       task_push_results(struct dcp_task* task, struct dcp_results* results);

#endif

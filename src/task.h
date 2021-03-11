#ifndef TASK_H
#define TASK_H

struct dcp_result;
struct dcp_task;
struct dcp_task_cfg;
struct sequence;

void                       task_add_result(struct dcp_task* task, struct dcp_result* result);
struct dcp_task_cfg const* task_cfg(struct dcp_task* task);
struct sequence const*     task_first_sequence(struct dcp_task* task);
struct sequence const*     task_next_sequence(struct dcp_task* task, struct sequence const* sequence);

#endif

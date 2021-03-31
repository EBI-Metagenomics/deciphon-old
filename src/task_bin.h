#ifndef TASK_BIN_H
#define TASK_BIN_H

#include <stdbool.h>

struct dcp_task;
struct task_bin;

typedef bool (*task_bin_collect_cb)(struct dcp_task* task, void* arg);

void             task_bin_collect(struct task_bin* bin);
struct task_bin* task_bin_create(task_bin_collect_cb collect, void* collect_arg);
int              task_bin_destroy(struct task_bin* bin);
int              task_bin_join(struct task_bin* bin);
void             task_bin_put(struct task_bin* bin, struct dcp_task* task);
int              task_bin_start(struct task_bin* bin);
void             task_bin_stop(struct task_bin* bin);

#endif

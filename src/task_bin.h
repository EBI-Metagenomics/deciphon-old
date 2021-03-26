#ifndef TASK_BIN_H
#define TASK_BIN_H

#include "containers/stack.h"

struct dcp_task;

struct task_bin
{
    unsigned     put_stack;
    unsigned     last_collect;
    struct stack stacks[2];
};

typedef bool (*collect_task_cb)(struct dcp_task* task, void* arg);

void task_bin_collect(struct task_bin* bin, collect_task_cb collect_task, void* arg);
void task_bin_init(struct task_bin* bin);
void task_bin_put(struct task_bin* bin, struct dcp_task* task);

#endif

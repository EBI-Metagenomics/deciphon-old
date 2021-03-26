#include "task_bin.h"
#include "task.h"
#include <ck_pr.h>

void task_bin_collect(struct task_bin* bin, collect_task_cb collect_task, void* arg)
{
    unsigned put_stack = ck_pr_load_uint(&bin->put_stack);
    if (put_stack == bin->last_collect)
        return;

    unsigned collect_stack = (put_stack + 1) % 2;

    struct stack* stack = bin->stacks + collect_stack;
    struct stack  remain = STACK_INIT(remain);
    while (!stack_empty(stack)) {

        struct snode*    n = stack_pop(stack);
        struct dcp_task* task = CONTAINER_OF(n, struct dcp_task, bin_node);
        if (!collect_task(task, arg))
            stack_push(&remain, &task->bin_node);
    }

    while (!stack_empty(&remain))
        stack_push(stack, stack_pop(&remain));

    if (stack_empty(stack)) {
        bin->last_collect = collect_stack;
        ck_pr_store_uint(&bin->put_stack, collect_stack);
    }
}

void task_bin_init(struct task_bin* bin)
{
    bin->put_stack = 0;
    bin->last_collect = 0;
    stack_init(bin->stacks + 0);
    stack_init(bin->stacks + 1);
}

void task_bin_put(struct task_bin* bin, struct dcp_task* task)
{
    unsigned i = ck_pr_load_uint(&bin->put_stack) % 2;
    stack_push(bin->stacks + i, &task->bin_node);
    ck_pr_add_uint(&bin->put_stack, 2);
}

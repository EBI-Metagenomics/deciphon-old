#include "task_bin.h"
#include "containers/stack.h"
#include "dthread.h"
#include "task.h"
#include "util.h"
#include <ck_pr.h>
#include <stdlib.h>

struct thread
{
    pthread_cond_t  cond;
    pthread_mutex_t mutex;
    bool            stop_signal;
    pthread_t       thread;
};

struct task_bin
{
    task_bin_collect_cb collect;
    void*               collect_arg;
    unsigned            put_stack;
    unsigned            last_collect;
    struct stack        stacks[2];
    struct thread       thread;
};

static void  collect_garbage(struct task_bin* bin);
static void  force_collection(struct task_bin* bin);
static void* thread_routine(void* task_bin_addr);

struct task_bin* task_bin_create(task_bin_collect_cb collect, void* collect_arg)
{
    struct task_bin* bin = malloc(sizeof(*bin));
    bin->collect = collect;
    bin->collect_arg = collect_arg;
    bin->put_stack = 0;
    bin->last_collect = 0;
    stack_init(bin->stacks + 0);
    stack_init(bin->stacks + 1);
    dthread_mutex_init(&bin->thread.mutex);
    dthread_cond_init(&bin->thread.cond);
    bin->thread.stop_signal = 0;
    return bin;
}

void task_bin_destroy(struct task_bin* bin)
{
    BUG(!bin->thread.stop_signal);
    force_collection(bin);
    pthread_cond_destroy(&bin->thread.cond);
    pthread_mutex_destroy(&bin->thread.mutex);
    free(bin);
}

void task_bin_join(struct task_bin* bin) { dthread_join(bin->thread.thread); }

void task_bin_put(struct task_bin* bin, struct dcp_task* task)
{
    unsigned i = ck_pr_load_uint(&bin->put_stack) % 2;
    stack_push(bin->stacks + i, &task->bin_node);
    ck_pr_add_uint(&bin->put_stack, 2);
}

void task_bin_start(struct task_bin* bin) { dthread_create(&bin->thread.thread, thread_routine, (void*)bin); }

void task_bin_stop(struct task_bin* bin)
{
    dthread_lock(&bin->thread.mutex);
    bin->thread.stop_signal = true;
    dthread_unlock(&bin->thread.mutex);
    pthread_cond_signal(&bin->thread.cond);
}

static void collect_garbage(struct task_bin* bin)
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
        if (!bin->collect(task, bin->collect_arg))
            stack_push(&remain, &task->bin_node);
    }

    while (!stack_empty(&remain))
        stack_push(stack, stack_pop(&remain));

    if (stack_empty(stack)) {
        bin->last_collect = collect_stack;
        ck_pr_store_uint(&bin->put_stack, collect_stack);
    }
}

static void force_collection(struct task_bin* bin)
{
    ck_pr_add_uint(&bin->put_stack, 2);
    collect_garbage(bin);
}

static void* thread_routine(void* task_bin_addr)
{
    struct task_bin* bin = task_bin_addr;
    struct thread*   thread = &bin->thread;

    dthread_lock(&thread->mutex);
    bool stop_signal = thread->stop_signal;
    while (!stop_signal) {

        dthread_timedwait(&thread->cond, &thread->mutex, 3);
        collect_garbage(bin);
        stop_signal = thread->stop_signal;
    }
    dthread_unlock(&thread->mutex);

    return NULL;
}

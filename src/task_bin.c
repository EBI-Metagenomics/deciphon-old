#include "task_bin.h"
#include "clock.h"
#include "containers/stack.h"
#include "task.h"
#include "util.h"
#include <ck_pr.h>
#include <pthread.h>
#include <stdlib.h>

#define SLEEP_DURATION 5

struct thread
{
    struct clock* clock;
    int           stop_signal;
    int           active;
    pthread_t     id;
};

struct task_bin
{
    task_bin_collect_cb collect;
    void*               collect_arg;
    unsigned            epoch;
    unsigned            last_collect;
    struct stack        stacks[2];
    struct thread       thread;
};

struct stacks_state
{
    unsigned collect_stack; /* (put_stack + 1) % 2 */
    unsigned put_stack;     /* epoch % 2 */
    unsigned epoch;
};

static inline void         advance_epoch(struct task_bin* bin) { ck_pr_add_uint(&bin->epoch, 2); }
static void                collect_garbage(struct task_bin* bin);
static void                destroy_tasks(struct task_bin* bin);
static void                force_collection(struct task_bin* bin);
static inline void         thread_activate(struct thread* thr) { ck_pr_store_int(&thr->active, 1); }
static inline bool         thread_active(struct thread const* thr) { return ck_pr_load_int(&thr->active); }
static inline void         thread_deactivate(struct thread* thr) { ck_pr_store_int(&thr->active, 0); }
static inline void         thread_deinit(struct thread* thr) { clock_destroy(thr->clock); }
static int                 thread_init(struct thread* thr);
static void                thread_stop(struct thread* thr);
static int                 thread_timedstop(struct thread* thr);
static void*               main_thread(void* task_bin);
static inline void         set_epoch(struct task_bin* bin, unsigned epoch) { ck_pr_store_uint(&bin->epoch, epoch); }
static struct stacks_state stacks_state(struct task_bin const* bin);

void task_bin_collect(struct task_bin* bin) { collect_garbage(bin); }

struct task_bin* task_bin_create(task_bin_collect_cb collect, void* collect_arg)
{
    struct task_bin* bin = malloc(sizeof(*bin));
    bin->collect = collect;
    bin->collect_arg = collect_arg;
    bin->epoch = 0;
    bin->last_collect = 0;
    stack_init(bin->stacks + 0);
    stack_init(bin->stacks + 1);
    if (thread_init(&bin->thread)) {
        free(bin);
        return NULL;
    }
    return bin;
}

int task_bin_destroy(struct task_bin* bin)
{
    if (thread_active(&bin->thread)) {
        warn("destroying active task_bin");
        if (thread_timedstop(&bin->thread)) {
            error("failed to destroy task_bin");
            return 1;
        }
    }
    force_collection(bin);
    destroy_tasks(bin);
    thread_deinit(&bin->thread);
    free(bin);
    return 0;
}

int task_bin_join(struct task_bin* bin) { return pthread_join(bin->thread.id, NULL); }

void task_bin_put(struct task_bin* bin, struct dcp_task* task)
{
    struct stacks_state ss = stacks_state(bin);
    stack_push(bin->stacks + ss.put_stack, &task->bin_node);
    advance_epoch(bin);
}

int task_bin_start(struct task_bin* bin)
{
    thread_activate(&bin->thread);
    if (pthread_create(&bin->thread.id, NULL, main_thread, (void*)bin)) {
        error("could not spawn thread_main");
        thread_deactivate(&bin->thread);
        return 1;
    }
    return 0;
}

void task_bin_stop(struct task_bin* bin) { thread_stop(&bin->thread); }

static void collect_garbage(struct task_bin* bin)
{
    struct stacks_state ss = stacks_state(bin);
    if (ss.put_stack == bin->last_collect)
        return;

    struct stack* stack = bin->stacks + ss.collect_stack;
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
        bin->last_collect = ss.collect_stack;
        set_epoch(bin, ss.collect_stack);
    }
}

static void destroy_tasks(struct task_bin* bin)
{
    for (unsigned i = 0; i < ARRAY_SIZE(bin->stacks); ++i) {
        struct stack* stack = bin->stacks + i;

        while (!stack_empty(stack)) {
            struct snode*    n = stack_pop(stack);
            struct dcp_task* task = CONTAINER_OF(n, struct dcp_task, bin_node);
            dcp_task_destroy(task);
        }
    }
}

static void force_collection(struct task_bin* bin)
{
    advance_epoch(bin);
    collect_garbage(bin);
}

static int thread_init(struct thread* thr)
{
    if (!(thr->clock = clock_create()))
        return 1;
    thr->active = 0;
    thr->stop_signal = 0;
    return 0;
}

static void thread_stop(struct thread* thr)
{
    ck_pr_store_int(&thr->stop_signal, 1);
    clock_wakeup(thr->clock);
}

static int thread_timedstop(struct thread* thr)
{
    for (unsigned i = 0; i < 3; ++i) {
        thread_stop(thr);
        dcp_sleep(100 + i * 250);
        if (!thread_active(thr))
            return 0;
    }
    return 1;
}

static void* main_thread(void* task_bin)
{
    struct task_bin* bin = task_bin;
    struct thread*   thread = &bin->thread;

    while (!ck_pr_load_int(&thread->stop_signal)) {
        clock_sleep(thread->clock, SLEEP_DURATION * 1000);
        collect_garbage(bin);
    }

    thread_deactivate(thread);
    pthread_exit(NULL);
    return NULL;
}

static struct stacks_state stacks_state(struct task_bin const* bin)
{
    unsigned epoch = ck_pr_load_uint(&bin->epoch);
    return (struct stacks_state){(epoch + 1) % 2, (epoch + 0) % 2, epoch};
}

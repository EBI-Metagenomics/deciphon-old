#include "gc.h"
#include "signal.h"
#include "util.h"
#include <ck_pr.h>
#include <pthread.h>

struct gc
{
    pthread_t       thread;
    pthread_cond_t  cond;
    pthread_mutex_t mutex;
    int             signal;
    gc_collect_cb   collect;
    void*           collect_arg;
};

static void*       loop(void* gc_addr);
static inline bool sigstop(struct gc* gc) { return ck_pr_load_int(&gc->signal) == SIGNAL_STOP; }

void gc_collect(struct gc* gc) { pthread_cond_signal(&gc->cond); }

struct gc* gc_create(gc_collect_cb collect, void* collect_arg)
{
    struct gc* gc = malloc(sizeof(*gc));
    BUG(pthread_mutex_init(&gc->mutex, NULL));
    BUG(pthread_cond_init(&gc->cond, NULL));
    gc->signal = SIGNAL_NONE;
    gc->collect = collect;
    gc->collect_arg = collect_arg;
    return gc;
}

void gc_destroy(struct gc* gc)
{
    BUG(pthread_cond_destroy(&gc->cond));
    BUG(pthread_mutex_destroy(&gc->mutex));
    free(gc);
}

void gc_join(struct gc* gc)
{
    void* ret = NULL;
    BUG(pthread_join(gc->thread, &ret));
}

void gc_start(struct gc* gc) { BUG(pthread_create(&gc->thread, NULL, loop, (void*)gc)); }

void gc_stop(struct gc* gc)
{
    ck_pr_store_int(&gc->signal, SIGNAL_STOP);
    pthread_cond_signal(&gc->cond);
}

static void* loop(void* gc_addr)
{
    struct gc* gc = gc_addr;
    BUG(pthread_mutex_lock(&gc->mutex));

    while (!sigstop(gc)) {

        struct timespec wait_time = {0, 0};
        BUG(clock_gettime(CLOCK_REALTIME, &wait_time));
        wait_time.tv_sec += 1;
        pthread_cond_timedwait(&gc->cond, &gc->mutex, &wait_time);

        gc->collect(gc->collect_arg);
    }
    gc->collect(gc->collect_arg);

    BUG(pthread_mutex_unlock(&gc->mutex));
    return NULL;
}

#include "dthread.h"
#include "util.h"

void dthread_cond_destroy(pthread_cond_t* cond) { BUG(pthread_cond_destroy(cond)); }

void dthread_cond_init(pthread_cond_t* restrict cond) { BUG(pthread_cond_init(cond, NULL)); }

void dthread_create(pthread_t* thread, void* (*start_routine)(void*), void* arg)
{
    BUG(pthread_create(thread, NULL, start_routine, arg));
}

void dthread_join(pthread_t thread) { BUG(pthread_join(thread, NULL)); }

void dthread_lock(pthread_mutex_t* mutex) { BUG(pthread_mutex_lock(mutex)); }

void dthread_mutex_destroy(pthread_mutex_t* mutex) { BUG(pthread_mutex_destroy(mutex)); }

void dthread_mutex_init(pthread_mutex_t* restrict mutex) { BUG(pthread_mutex_init(mutex, NULL)); }

int dthread_timedwait(pthread_cond_t* cond, pthread_mutex_t* mutex, time_t seconds)
{
    struct timespec wait_time = {0, 0};
    if (clock_gettime(CLOCK_REALTIME, &wait_time)) {
        warn("clock_gettime failed");
        return 1;
    }
    wait_time.tv_sec += seconds;
    if (pthread_cond_timedwait(cond, mutex, &wait_time)) {
        warn("pthread_cond_timedwait failed");
        return 1;
    }
    return 0;
}

void dthread_unlock(pthread_mutex_t* mutex) { BUG(pthread_mutex_unlock(mutex)); }

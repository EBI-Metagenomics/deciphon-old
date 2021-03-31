#include "clock.h"
#include "util.h"
#include <pthread.h>

struct clock
{
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
};

struct clock* clock_create(void)
{
    struct clock* clock = malloc(sizeof(*clock));
    if (pthread_mutex_init(&clock->mutex, NULL)) {
        error("could not mutex_init");
        free(clock);
        return NULL;
    }
    if (pthread_cond_init(&clock->cond, NULL)) {
        error("could not cond_init");
        free(clock);
        return NULL;
    }
    return clock;
}

void clock_destroy(struct clock* clock)
{
    if (pthread_cond_destroy(&clock->cond))
        warn("cond_destroy failed");
    if (pthread_mutex_destroy(&clock->mutex))
        warn("mutex_destroy failed");
    free(clock);
}

void clock_sleep(struct clock* clock, unsigned milliseconds)
{
    if (pthread_mutex_lock(&clock->mutex)) {
        warn("mutex_lock failed");
        return;
    }

    struct timespec wait_time = {0, 0};
    if (clock_gettime(CLOCK_REALTIME, &wait_time)) {
        warn("clock_gettime failed");
        return;
    }
    wait_time.tv_sec += milliseconds / 1000;
    wait_time.tv_nsec += (milliseconds % 1000) * 1000000;

    if (pthread_cond_timedwait(&clock->cond, &clock->mutex, &wait_time)) {
        warn("cond_timedwait failed");
        return;
    }
    if (pthread_mutex_unlock(&clock->mutex))
        warn("mutex_unlock failed");
}

void clock_wakeup(struct clock* clock)
{
    if (pthread_mutex_lock(&clock->mutex)) {
        warn("mutex_lock failed");
        return;
    }
    if (pthread_cond_signal(&clock->cond)) {
        warn("cond_signal failed");
        return;
    }
    if (pthread_mutex_unlock(&clock->mutex))
        warn("mutex_unlock failed");
}

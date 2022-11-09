#ifndef CORE_SPINLOCK_H
#define CORE_SPINLOCK_H

#include <stdatomic.h>

typedef atomic_flag spinlock_t;

#define SPINLOCK_INIT                                                          \
    (spinlock_t)                                                               \
    {                                                                          \
        0                                                                      \
    }

static inline void spinlock_lock(spinlock_t *lock)
{
    while (atomic_flag_test_and_set(lock))
        ;
}

static inline void spinlock_unlock(spinlock_t *lock)
{
    atomic_flag_clear(lock);
}

#endif

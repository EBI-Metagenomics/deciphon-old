#ifndef CORE_ATOMIC_H
#define CORE_ATOMIC_H

#include <stdatomic.h>

#define atomic_release(object, value)                                          \
    atomic_store_explicit((object), (value), memory_order_release)

#define atomic_consume(object)                                                 \
    atomic_load_explicit((object), memory_order_consume)

#endif

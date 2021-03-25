#ifndef CONTAINERS_CONTAINER_H
#define CONTAINERS_CONTAINER_H

#include <stddef.h>

#define CONTAINER_OF(ptr, type, member) ((type*)((size_t)(void*)(ptr)-offsetof(type, member)))

#define CONTAINER_OF_OR_NULL(ptr, type, member)                                                                        \
    ({                                                                                                                 \
        void* ptr__ = (ptr);                                                                                           \
        ptr__ ? CONTAINER_OF(ptr__, type, member) : NULL;                                                              \
    })

#endif

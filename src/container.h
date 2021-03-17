#ifndef CONTAINER_H
#define CONTAINER_H

#include <stddef.h>

#define CONTAINER_OF(ptr, type, member) ((type*)((size_t)(void*)(ptr)-offsetof(type, member)))

#endif

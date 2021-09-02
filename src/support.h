#ifndef SUPPORT_H
#define SUPPORT_H

#include <stdlib.h>

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define FILL(size, val)                                                        \
    {                                                                          \
        [0 ...(size) - 1] = val                                                \
    }

#define xdel(x)                                                                \
    do                                                                         \
    {                                                                          \
        x = __xdel(x);                                                         \
    } while (0);

static inline void *__xdel(void const *ptr)
{
    if (ptr) free((void *)ptr);
    return NULL;
}

#endif

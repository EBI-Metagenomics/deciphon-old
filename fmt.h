#ifndef FMT_H
#define FMT_H

#include "array_size.h"
#include <stddef.h>

int fmt(char *dst, size_t dsize, char const *fmt, ...)
    __attribute__((format(printf, 3, 4)));

#define FMT(dst, format, ...) fmt((dst), array_size(dst), (format), __VA_ARGS__)

#endif

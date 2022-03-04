#ifndef DECIPHON_UTIL_STRLCPY_H
#define DECIPHON_UTIL_STRLCPY_H

#include <stddef.h>

#ifndef HAVE_STRLCPY
size_t strlcpy(char *dst, const char *src, size_t dsize);
#endif

#endif

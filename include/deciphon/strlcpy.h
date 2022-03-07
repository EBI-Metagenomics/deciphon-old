#ifndef DECIPHON_STRLCPY_H
#define DECIPHON_STRLCPY_H

#ifndef HAVE_STRLCPY
#include <stddef.h>
size_t strlcpy(char *dst, const char *src, size_t dsize);
#else
#include "string.h"
#endif

#endif

#ifndef SAFE_H
#define SAFE_H

#include "common/export.h"
#include <stddef.h>

EXPORT size_t safe_strcpy(char *restrict dst, char const *restrict src,
                          size_t dst_size);

#endif

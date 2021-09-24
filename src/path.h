#ifndef PATH_H
#define PATH_H

#include <stdbool.h>
#include <stddef.h>
#define _POSIX_C_SOURCE 1
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

bool path_change_or_add_ext(char *str, size_t max_size, char const *ext);

#endif

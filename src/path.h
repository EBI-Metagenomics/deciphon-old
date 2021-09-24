#ifndef PATH_H
#define PATH_H

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

bool path_change_or_add_ext(char *str, size_t max_size, char const *ext);

#endif

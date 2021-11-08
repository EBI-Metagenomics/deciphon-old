#ifndef PATH_H
#define PATH_H

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

#define PATH_TEMP_TEMPLATE "/tmp/dcpXXXXXX"
#define PATH_TEMP_DECLARE(n) char n[sizeof PATH_TEMP_TEMPLATE]
#define PATH_TEMP_DEFINE(n) PATH_TEMP_DECLARE(n) = PATH_TEMP_TEMPLATE;

bool path_change_or_add_ext(char str[static 1], size_t max_size,
                            char const ext[static 1]);

#endif

#ifndef PATH_H
#define PATH_H

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

/* Windows seems to have limited it to 260 characters in the past */
#define PATH_SIZE 261

#define PATH_TEMP_TEMPLATE "/tmp/dcpXXXXXX"
#define PATH_TEMP_DECLARE(n) char n[sizeof PATH_TEMP_TEMPLATE]

bool path_change_or_add_ext(char str[static 1], size_t max_size,
                            char const ext[static 1]);

#endif

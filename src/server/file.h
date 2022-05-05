#ifndef SERVER_FILE_H
#define SERVER_FILE_H

#include "deciphon/core/rc.h"
#include <stdint.h>

typedef enum rc (*file_fetch_func_t)(char const *filename, int64_t xxh3);

enum rc file_ensure_local(char const *filename, int64_t xxh3,
                          file_fetch_func_t fetch);

#endif

#ifndef DECIPHON_CORE_FILE_H
#define DECIPHON_CORE_FILE_H

#include "deciphon/core/rc.h"
#include <stdint.h>

typedef enum rc(file_download_fn_t)(char const *filename, void *data);

enum rc file_ensure_local(char const *filename, int64_t xxh3,
                          file_download_fn_t *, void *data);

#endif

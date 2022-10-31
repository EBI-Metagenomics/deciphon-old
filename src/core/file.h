#ifndef CORE_FILE_H
#define CORE_FILE_H

#include "core/rc.h"
#include <stdint.h>
#include <stdio.h>

typedef enum rc(file_download_fn_t)(char const *filename, void *data);

enum rc file_ensure_local(char const *filename, long xxh3, file_download_fn_t *,
                          void *data);
enum rc file_hash(char const *filepath, long *hash);
enum rc file_phash(FILE *fp, long *hash);

#endif

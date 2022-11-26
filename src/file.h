#ifndef FILE_H
#define FILE_H

#include "rc.h"
#include <stdint.h>
#include <stdio.h>

typedef enum rc(file_download_fn_t)(char const *filename, void *data);

enum rc file_ensure_local(char const *filename, long xxh3, file_download_fn_t *,
                          void *data);
enum rc file_hash(char const *filepath, long *hash);
enum rc file_phash(FILE *fp, long *hash);

#endif

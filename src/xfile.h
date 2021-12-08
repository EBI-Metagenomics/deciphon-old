#ifndef XFILE_H
#define XFILE_H

#include "dcp_limits.h"
#include "rc.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define XFILE_PATH_TEMP_TEMPLATE "/tmp/dcpXXXXXX"

struct xfile_tmp
{
    char path[sizeof XFILE_PATH_TEMP_TEMPLATE];
    FILE *fp;
};

enum rc xfile_tmp_open(struct xfile_tmp *file);
enum rc xfile_tmp_rewind(struct xfile_tmp *file);
void xfile_tmp_destroy(struct xfile_tmp *file);

enum rc xfile_copy(FILE *restrict dst, FILE *restrict src);
bool xfile_is_readable(char const *filepath);
enum rc xfile_mktemp(char *filepath);

enum rc xfile_hash(FILE *restrict fp, uint64_t *hash);

bool xfile_set_path_ext(char *str, size_t max_size, char const *ext);
void xfile_basename(char *filename, char const *path);
void xfile_strip_path_ext(char *str);

FILE *xfile_open_from_fptr(FILE *fp, char const *mode);

#endif

#ifndef XFILE_H
#define XFILE_H

#include "dcp_limits.h"
#include "path.h"
#include "rc.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

struct xfile_tmp
{
    PATH_TEMP_DECLARE(path);
    FILE *fp;
};

enum rc xfile_tmp_open(struct xfile_tmp *file);
enum rc xfile_tmp_rewind(struct xfile_tmp *file);
void xfile_tmp_destroy(struct xfile_tmp *file);

enum rc xfile_copy(FILE *restrict dst, FILE *restrict src);
bool xfile_is_readable(char const *filepath);
enum rc xfile_mktemp(char *filepath);

enum rc xfile_hash(FILE *restrict fp, uint64_t *hash);

#endif

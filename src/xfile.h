#ifndef XFILE_H
#define XFILE_H

#include "dcp/limits.h"
#include "dcp/rc.h"
#include "path.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

struct xfile_tmp
{
    PATH_TEMP_DECLARE(path);
    FILE *fd;
};

enum dcp_rc xfile_tmp_open(struct xfile_tmp *file);
enum dcp_rc xfile_tmp_rewind(struct xfile_tmp *file);
void xfile_tmp_destroy(struct xfile_tmp *file);

enum dcp_rc xfile_copy(FILE *restrict dst, FILE *restrict src);
bool xfile_is_readable(char const filepath[DCP_PATH_SIZE]);
enum dcp_rc xfile_mktemp(char filepath[DCP_PATH_SIZE]);

enum dcp_rc xfile_hash(FILE *restrict fd, uint64_t *hash);

#endif

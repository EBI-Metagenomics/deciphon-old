#ifndef DCP_FILE_H
#define DCP_FILE_H

#include "dcp/rc.h"
#include <stdbool.h>
#include <stdio.h>

#define FILE_TMP_TEMPLATE "/tmp/dcpXXXXXX"

struct file_tmp
{
    char path[sizeof FILE_TMP_TEMPLATE];
};

#define FILE_TMP_INIT()                                                        \
    (struct file_tmp) { .path = FILE_TMP_TEMPLATE }

enum dcp_rc file_copy(FILE *restrict dst, FILE *restrict src);
enum dcp_rc file_empty(char const *filepath);
bool file_readable(char const *filepath);
enum dcp_rc file_tmp_mk(struct file_tmp *tmp);
enum dcp_rc file_tmp_rm(struct file_tmp const *tmp);
enum dcp_rc file_write_newline(FILE *restrict fd);

#endif

#ifndef DCP_FILE_H
#define DCP_FILE_H

#include "dcp/rc.h"
#include <stdbool.h>
#include <stdio.h>

enum dcp_rc file_copy(FILE *restrict dst, FILE *restrict src);
enum dcp_rc file_empty(char const *filepath);
bool file_readable(char const *filepath);

#endif

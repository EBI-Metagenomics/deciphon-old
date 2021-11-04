#ifndef XFILE_H
#define XFILE_H

#include "dcp/rc.h"
#include <stdbool.h>
#include <stdio.h>

enum dcp_rc xfile_copy(FILE *restrict dst, FILE *restrict src);
bool xfile_is_readable(char const filepath[static 1]);
enum dcp_rc xfile_mktemp(char filepath[static 1]);

#endif

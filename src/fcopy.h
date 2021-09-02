#ifndef FCOPY_H
#define FCOPY_H

#include "dcp/rc.h"
#include <stdio.h>

enum dcp_rc fcopy(FILE *restrict dst, FILE *restrict src);

#endif

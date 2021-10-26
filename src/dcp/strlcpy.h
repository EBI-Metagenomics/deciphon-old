#ifndef DCP_STRLCPY_H
#define DCP_STRLCPY_H

#include "dcp/export.h"
#include <stddef.h>

DCP_API size_t dcp_strlcpy(char *dst, const char *src, size_t len);

#endif

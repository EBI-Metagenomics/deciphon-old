#ifndef DCP_STR_H
#define DCP_STR_H

#include "dcp/export.h"
#include <stdint.h>

struct dcp_string;

DCP_API char const* dcp_string_data(struct dcp_string const* string);
DCP_API uint32_t    dcp_string_size(struct dcp_string const* string);

#endif

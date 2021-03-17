#ifndef DCP_STR_H
#define DCP_STR_H

#include <stdint.h>

struct dcp_string;

char const* dcp_string_data(struct dcp_string const* string);
uint32_t    dcp_string_size(struct dcp_string const* string);

#endif

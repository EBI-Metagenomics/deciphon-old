#ifndef DECIPHON_INPUT_H
#define DECIPHON_INPUT_H

#include "deciphon/export.h"
#include <inttypes.h>

struct dcp_input;
struct dcp_partition;

DCP_API struct dcp_input*     dcp_input_create(char const* filepath);
DCP_API struct dcp_partition* dcp_input_create_partition(struct dcp_input const* input, uint32_t partition,
                                                         uint32_t npartitions);
DCP_API int                   dcp_input_destroy(struct dcp_input* input);

#endif

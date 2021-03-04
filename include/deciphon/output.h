#ifndef DECIPHON_OUTPUT_H
#define DECIPHON_OUTPUT_H

#include "deciphon/export.h"
#include <inttypes.h>

struct dcp_profile;
struct dcp_output;

DCP_API int                dcp_output_close(struct dcp_output* output);
DCP_API struct dcp_output* dcp_output_create(char const* filepath);
DCP_API int                dcp_output_destroy(struct dcp_output* output);
DCP_API int                dcp_output_write(struct dcp_output* output, struct dcp_profile const* prof);

#endif

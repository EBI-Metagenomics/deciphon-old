#ifndef DCP_INPUT_H
#define DCP_INPUT_H

#include "dcp/export.h"
#include <stdbool.h>
#include <stdint.h>

struct dcp_input;
struct dcp_metadata;

DCP_API int                        dcp_input_close(struct dcp_input* input);
DCP_API struct dcp_input*          dcp_input_create(char const* filepath);
DCP_API int                        dcp_input_destroy(struct dcp_input* input);
DCP_API bool                       dcp_input_end(struct dcp_input const* input);
DCP_API struct dcp_metadata const* dcp_input_metadata(struct dcp_input const* input, uint32_t profid);
DCP_API uint32_t                   dcp_input_nprofiles(struct dcp_input const* input);
DCP_API struct dcp_profile const*  dcp_input_read(struct dcp_input* input);
DCP_API int                        dcp_input_reset(struct dcp_input* input);

#endif

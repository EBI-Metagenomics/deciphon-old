#include "str.h"
#include "dcp/dcp.h"

char const* dcp_string_data(struct dcp_string const* string) { return stream_data(&string->stream); }

uint32_t dcp_string_size(struct dcp_string const* string) { return (uint32_t)stream_size(&string->stream); }

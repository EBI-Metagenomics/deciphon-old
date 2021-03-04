#ifndef DECIPHON_RESULT_H
#define DECIPHON_RESULT_H

#include "deciphon/export.h"
#include "imm/imm.h"

struct dcp_result;

DCP_API imm_float dcp_result_alt_loglik(struct dcp_result const* result);
DCP_API void      dcp_result_destroy(struct dcp_result const* result);
DCP_API imm_float dcp_result_null_loglik(struct dcp_result const* result);
DCP_API uint32_t  dcp_result_profid(struct dcp_result const* result);

#endif

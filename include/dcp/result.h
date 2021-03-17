#ifndef DCP_RESULT_H
#define DCP_RESULT_H

#include "dcp/export.h"
#include "dcp/model.h"
#include "imm/imm.h"

struct dcp_result;
struct dcp_string;

DCP_API struct dcp_string const* dcp_result_codons(struct dcp_result const* result, enum dcp_model_t model);
DCP_API imm_float                dcp_result_loglik(struct dcp_result const* result, enum dcp_model_t model);
DCP_API struct dcp_string const* dcp_result_path(struct dcp_result const* result, enum dcp_model_t model);
DCP_API uint32_t                 dcp_result_profid(struct dcp_result const* result);
DCP_API uint32_t                 dcp_result_seqid(struct dcp_result const* result);

#endif

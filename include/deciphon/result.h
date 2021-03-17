#ifndef DECIPHON_RESULT_H
#define DECIPHON_RESULT_H

#include "deciphon/export.h"
#include "imm/imm.h"

struct dcp_result;

enum dcp_model
{
    DCP_ALT,
    DCP_NULL
};

#define DCP_NMODELS 2

DCP_API char const* dcp_result_codons(struct dcp_result const* result, enum dcp_model model);
DCP_API void        dcp_result_destroy(struct dcp_result const* result);
DCP_API imm_float   dcp_result_loglik(struct dcp_result const* result, enum dcp_model model);
DCP_API char const* dcp_result_path(struct dcp_result const* result, enum dcp_model model);
DCP_API uint32_t    dcp_result_profid(struct dcp_result const* result);
DCP_API uint32_t    dcp_result_seqid(struct dcp_result const* result);

#endif

#include "result.h"
#include "deciphon/deciphon.h"
#include "free.h"

imm_float dcp_result_alt_loglik(struct dcp_result const* result) { return result->alt_loglik; }

void dcp_result_destroy(struct dcp_result const* result)
{
    imm_result_destroy(result->null_result);
    imm_result_destroy(result->alt_result);
    free_c(result);
}

imm_float dcp_result_null_loglik(struct dcp_result const* result) { return result->null_loglik; }

uint32_t dcp_result_profid(struct dcp_result const* result) { return result->profid; }

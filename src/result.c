#include "result.h"
#include "deciphon/deciphon.h"
#include <stdlib.h>

imm_float dcp_result_alt_loglik(struct dcp_result const* result) { return result->alt_loglik; }

struct imm_result const* dcp_result_alt_result(struct dcp_result const* result) { return result->alt_result; }

char const* dcp_result_alt_stream(struct dcp_result const* result) { return result->alt_stream; }

void dcp_result_destroy(struct dcp_result const* result)
{
    imm_result_destroy(result->null_result);
    imm_result_destroy(result->alt_result);
    free((void*)result->alt_stream);
    free((void*)result->null_stream);
    free((void*)result);
}

imm_float dcp_result_null_loglik(struct dcp_result const* result) { return result->null_loglik; }

struct imm_result const* dcp_result_null_result(struct dcp_result const* result) { return result->null_result; }

char const* dcp_result_null_stream(struct dcp_result const* result) { return result->null_stream; }

uint32_t dcp_result_profid(struct dcp_result const* result) { return result->profid; }

uint32_t dcp_result_seqid(struct dcp_result const* result) { return result->seqid; }

#include "result.h"
#include "free.h"

void result_destroy(struct dcp_result const* result)
{
    imm_result_destroy(result->null_result);
    imm_result_destroy(result->alt_result);
    free_c(result);
}

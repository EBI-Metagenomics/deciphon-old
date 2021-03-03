#ifndef RESULT_H
#define RESULT_H

#include "imm/imm.h"

struct dcp_result
{
    uint32_t                 profid;
    struct imm_result const* null_result;
    imm_float                null_loglik;
    struct imm_result const* alt_result;
    imm_float                alt_loglik;
};

void result_destroy(struct dcp_result const* result);

#endif

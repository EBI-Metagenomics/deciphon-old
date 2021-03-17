#ifndef MODEL_H
#define MODEL_H

#include "imm/imm.h"
#include "str.h"

struct dcp_model
{
    imm_float                loglik;
    struct imm_result const* result;
    struct dcp_string        path;
    struct dcp_string        codons;
};

static inline void model_set_loglik(struct dcp_model* model, imm_float loglik) { model->loglik = loglik; }
static inline void model_set_result(struct dcp_model* model, struct imm_result const* r) { model->result = r; }

#endif

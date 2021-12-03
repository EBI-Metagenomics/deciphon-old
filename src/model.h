#ifndef MODEL_H
#define MODEL_H

#include "imm/imm.h"
#include "str.h"

enum dcp_model
{
    DCP_MODEL_ALT,
    DCP_MODEL_NULL
};

extern enum dcp_model const dcp_models[2];

struct model
{
    imm_float loglik;
    struct imm_result const *result;
    struct dcp_string path;
    struct dcp_string codons;
};

static inline void model_set_loglik(struct model *model, imm_float loglik)
{
    model->loglik = loglik;
}
static inline void model_set_result(struct model *model,
                                    struct imm_result const *r)
{
    model->result = r;
}

#endif

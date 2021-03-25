#include "result.h"
#include "dcp/dcp.h"

struct dcp_string const* dcp_result_codons(struct dcp_result const* result, enum dcp_model_t model)
{
    return &result->models[model].codons;
}

imm_float dcp_result_loglik(struct dcp_result const* result, enum dcp_model_t model)
{
    return result->models[model].loglik;
}

struct dcp_string const* dcp_result_path(struct dcp_result const* result, enum dcp_model_t model)
{
    return &result->models[model].path;
}

uint32_t dcp_result_profid(struct dcp_result const* result) { return result->profid; }

uint32_t dcp_result_seqid(struct dcp_result const* result) { return result->seqid; }

void result_deinit(struct dcp_result* result)
{
    for (unsigned i = 0; i <= ARRAY_SIZE(dcp_models); ++i) {
        if (result->models[i].result)
            imm_result_destroy(result->models[i].result);
        string_deinit(&result->models[i].path);
        string_deinit(&result->models[i].codons);
    }
}

void result_init(struct dcp_result* result)
{
    for (unsigned i = 0; i <= ARRAY_SIZE(dcp_models); ++i) {
        result->models[i].result = NULL;
        string_init(&result->models[i].path);
        string_init(&result->models[i].codons);
    }
}

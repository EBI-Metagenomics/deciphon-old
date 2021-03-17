#include "result.h"
#include "deciphon/deciphon.h"
#include <stdlib.h>

char const* dcp_result_codons(struct dcp_result const* result, enum dcp_model model)
{
    return result->models[model].stream.codons;
}

void dcp_result_destroy(struct dcp_result const* result)
{
    for (unsigned i = 0; i <= DCP_NMODELS; ++i) {
        if (result->models[i].result)
            imm_result_destroy(result->models[i].result);

        if (result->models[i].stream.codons)
            free((void*)result->models[i].stream.codons);

        if (result->models[i].stream.path)
            free((void*)result->models[i].stream.path);
    }

    free((void*)result);
}

imm_float dcp_result_loglik(struct dcp_result const* result, enum dcp_model model)
{
    return result->models[model].loglik;
}

char const* dcp_result_path(struct dcp_result const* result, enum dcp_model model)
{
    return result->models[model].stream.path;
}

uint32_t dcp_result_profid(struct dcp_result const* result) { return result->profid; }

uint32_t dcp_result_seqid(struct dcp_result const* result) { return result->seqid; }

struct dcp_result* result_create(void)
{
    struct dcp_result* r = malloc(sizeof(*r));
    for (unsigned i = 0; i <= DCP_NMODELS; ++i) {
        r->models[i].result = NULL;
        r->models[i].stream.path = NULL;
        r->models[i].stream.codons = NULL;
    }
    return r;
}

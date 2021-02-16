#include "deciphon/partition.h"
#include "nmm/nmm.h"
#include <stdlib.h>

struct dcp_partition
{
    char const*       filepath;
    struct nmm_input* input;
    uint64_t          start_offset;
    uint32_t          nmodels;
    uint32_t          curr_model;
};

struct dcp_partition* dcp_partition_create(char const* filepath, uint64_t start_offset,
                                           uint32_t nmodels)
{
    struct dcp_partition* p = malloc(sizeof(*p));
    p->filepath = filepath;
    p->input = nmm_input_create(filepath);
    nmm_input_fseek(p->input, (int64_t)start_offset);
    p->start_offset = start_offset;
    p->nmodels = nmodels;
    p->curr_model = 0;
    return p;
}

struct nmm_model const* dcp_partition_read(struct dcp_partition* part)
{
    if (dcp_partition_eof(part))
        return NULL;
    ++part->curr_model;
    return nmm_input_read(part->input);
}

int dcp_partition_reset(struct dcp_partition* part)
{
    part->curr_model = 0;
    return nmm_input_fseek(part->input, (int64_t)part->start_offset);
}

bool dcp_partition_eof(struct dcp_partition const* part)
{
    return part->curr_model >= part->nmodels;
}

uint32_t dcp_partition_nmodels(struct dcp_partition const* part) { return part->nmodels; }

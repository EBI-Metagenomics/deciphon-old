#include "deciphon/partition.h"
#include "free.h"
#include "nmm/nmm.h"
#include <stdlib.h>
#include <string.h>

struct dcp_partition
{
    char const*       filepath;
    struct nmm_input* input;
    uint64_t          start_offset;
    uint32_t          nprofiles;
    uint32_t          curr_profile;
};

struct dcp_partition* dcp_partition_create(char const* filepath, uint64_t start_offset, uint32_t nprofiles)
{
    struct dcp_partition* part = malloc(sizeof(*part));
    part->filepath = NULL;
    part->input = NULL;

    part->filepath = strdup(filepath);
    part->input = nmm_input_create(filepath);
    if (!part->input) {
        imm_error("could not nmm_input_create");
        goto err;
    }

    if (nmm_input_fseek(part->input, (int64_t)start_offset)) {
        imm_error("could not fseek to %" PRIu64, start_offset);
        goto err;
    }
    part->start_offset = start_offset;
    part->nprofiles = nprofiles;
    part->curr_profile = 0;
    return part;

err:
    if (part->filepath)
        free_c(part->filepath);

    if (part->input)
        nmm_input_destroy(part->input);

    return NULL;
}

void dcp_partition_destroy(struct dcp_partition const* part)
{
    if (part->filepath)
        free_c(part->filepath);

    if (part->input)
        nmm_input_destroy(part->input);
}

bool dcp_partition_end(struct dcp_partition const* part) { return part->curr_profile >= part->nprofiles; }

uint32_t dcp_partition_nprofiles(struct dcp_partition const* part) { return part->nprofiles; }

struct nmm_profile const* dcp_partition_read(struct dcp_partition* part)
{
    if (dcp_partition_end(part))
        return NULL;
    ++part->curr_profile;
    return nmm_input_read(part->input);
}

int dcp_partition_reset(struct dcp_partition* part)
{
    part->curr_profile = 0;
    if (nmm_input_fseek(part->input, (int64_t)part->start_offset)) {
        imm_error("could not fseek to %" PRIu64, part->start_offset);
        return 1;
    }
    return 0;
}

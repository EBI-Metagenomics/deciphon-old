#include "deciphon/deciphon.h"
#include "free.h"
#include "imath.h"
#include "minmax.h"
#include "nmm/nmm.h"
#include <stdlib.h>
#include <string.h>

struct dcp_input
{
    char const* filepath;
    FILE*       stream;
    uint32_t    nprofiles;
    uint64_t*   profile_offsets;
};

static inline uint32_t default_partition_size(uint32_t nprofiles, uint32_t npartitions);
static inline uint32_t nprofiles_of_partition(uint32_t nprofiles, uint32_t npartitions, uint32_t partition);

struct dcp_input* dcp_input_create(char const* filepath)
{
    struct dcp_input* input = malloc(sizeof(*input));
    input->filepath = strdup(filepath);
    input->stream = fopen(filepath, "rb");
    input->profile_offsets = NULL;
    if (!input->stream) {
        imm_error("could not open file %s for reading", filepath);
        goto err;
    }

    if (fread(&input->nprofiles, sizeof(input->nprofiles), 1, input->stream) < 1) {
        imm_error("failed to read from file");
        goto err;
    }

    input->profile_offsets = malloc(input->nprofiles * sizeof(*input->profile_offsets));
    if (fread(input->profile_offsets, sizeof(*input->profile_offsets), input->nprofiles, input->stream) < 1) {
        imm_error("failed to read from file");
        goto err;
    }

    return input;

err:
    if (input->filepath)
        free_c(input->filepath);

    if (input->stream)
        fclose(input->stream);

    if (input->profile_offsets)
        free_c(input->profile_offsets);

    free_c(input);
    return NULL;
}

struct dcp_partition* dcp_input_create_partition(struct dcp_input const* input, uint32_t partition,
                                                 uint32_t npartitions)
{
    IMM_BUG(input->nprofiles < npartitions);
    IMM_BUG(partition >= npartitions);
    uint32_t part_size = default_partition_size(input->nprofiles, npartitions);
    uint32_t index = part_size * partition;
    uint32_t n = nprofiles_of_partition(input->nprofiles, npartitions, partition);
    return dcp_partition_create(input->filepath, input->profile_offsets[index], n);
}

int dcp_input_destroy(struct dcp_input* input)
{
    free_c(input->filepath);
    fclose(input->stream);
    free_c(input->profile_offsets);
    free_c(input);
    return 0;
}

static inline uint32_t default_partition_size(uint32_t nprofiles, uint32_t npartitions)
{
    return imath_ceildiv(nprofiles, npartitions);
}

static inline uint32_t nprofiles_of_partition(uint32_t nprofiles, uint32_t npartitions, uint32_t partition)
{
    uint32_t part_size = default_partition_size(nprofiles, npartitions);
    return MIN(part_size, nprofiles - part_size * partition);
}

#include "deciphon/input.h"
#include "deciphon/partition.h"
#include "free.h"
#include "imm/imm.h"
#include "minmax.h"
#include <stdlib.h>
#include <string.h>

struct dcp_input
{
    char const* filepath;
    FILE*       stream;
    uint32_t    nmodels;
    uint64_t*   model_offsets;
};

static inline uint32_t ceildiv(uint32_t x, uint32_t y)
{
    IMM_BUG(y == 0);
    return (x + (y - 1)) / y;
}

static inline uint32_t infer_nmodels(uint32_t nelements, uint32_t npartitions, uint32_t partition)
{
    uint32_t part_size = ceildiv(nelements, npartitions);
    return MIN(part_size, nelements - part_size * partition);
}

struct dcp_input* dcp_input_create(char const* filepath)
{
    struct dcp_input* input = malloc(sizeof(*input));
    input->filepath = strdup(filepath);
    input->stream = fopen(filepath, "rb");
    if (!input->stream) {
        imm_error("could not open file %s for reading", filepath);
        goto ERROR;
    }

    fread(&input->nmodels, sizeof(input->nmodels), 1, input->stream);
    input->model_offsets = malloc(input->nmodels * sizeof(*input->model_offsets));
    fread(input->model_offsets, sizeof(*input->model_offsets), input->nmodels, input->stream);

    return input;

ERROR:

    return NULL;
}

struct dcp_partition* dcp_input_create_partition(struct dcp_input const* input, uint32_t partition,
                                                 uint32_t npartitions)
{
    IMM_BUG(input->nmodels < npartitions);
    IMM_BUG(partition >= npartitions);
    uint32_t part_size = ceildiv(input->nmodels, npartitions);
    uint32_t index = part_size * partition;
    return dcp_partition_create(input->filepath, input->model_offsets[index],
                                infer_nmodels(input->nmodels, npartitions, partition));
}

int dcp_input_destroy(struct dcp_input* input)
{
    free_c(input->filepath);
    fclose(input->stream);
    free_c(input->model_offsets);
    return 0;
}

#include "deciphon/deciphon.h"
#include "free.h"
#include "imath.h"
#include "minmax.h"
#include "nmm/nmm.h"
#include "profile.h"
#include <stdlib.h>
#include <string.h>

struct dcp_input
{
    char const*       filepath;
    FILE*             stream;
    struct nmm_input* nmm_input;
    uint32_t          nprofiles;
    uint64_t*         profile_offsets;
    uint32_t          curr_profile;
};

struct dcp_input* dcp_input_create(char const* filepath)
{
    struct dcp_input* input = malloc(sizeof(*input));
    input->filepath = strdup(filepath);
    input->stream = fopen(filepath, "rb");
    input->profile_offsets = NULL;
    input->nmm_input = NULL;
    if (!input->stream) {
        imm_error("could not open file %s for reading", filepath);
        goto err;
    }

    if (fread(&input->nprofiles, sizeof(input->nprofiles), 1, input->stream) < 1) {
        imm_error("failed to read nprofiles");
        goto err;
    }

    input->profile_offsets = malloc(input->nprofiles * sizeof(*input->profile_offsets));
    if (fread(input->profile_offsets, sizeof(*input->profile_offsets), input->nprofiles, input->stream) < 1) {
        imm_error("failed to read profile_offsets");
        goto err;
    }

    input->nmm_input = nmm_input_screate(filepath, input->stream);
    if (!input->nmm_input) {
        imm_error("could not nmm_input_screate");
        goto err;
    }

    input->curr_profile = 0;

    return input;

err:
    if (input->filepath)
        free_c(input->filepath);

    if (input->stream)
        fclose(input->stream);

    if (input->profile_offsets)
        free_c(input->profile_offsets);

    if (input->nmm_input)
        nmm_input_destroy(input->nmm_input);

    free_c(input);
    return NULL;
}

int dcp_input_destroy(struct dcp_input* input)
{
    free_c(input->filepath);
    fclose(input->stream);
    free_c(input->profile_offsets);
    nmm_input_destroy(input->nmm_input);
    free_c(input);
    return 0;
}

bool dcp_input_end(struct dcp_input const* input) { return input->curr_profile >= input->nprofiles; }

struct dcp_profile const* dcp_input_read(struct dcp_input* input)
{
    if (dcp_input_end(input))
        return NULL;
    return profile_create(input->curr_profile++, nmm_input_read(input->nmm_input));
}

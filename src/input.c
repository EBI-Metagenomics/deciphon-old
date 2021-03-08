#include "deciphon/deciphon.h"
#include "free.h"
#include "imath.h"
#include "metadata.h"
#include "minmax.h"
#include "nmm/nmm.h"
#include "profile.h"
#include <stdlib.h>
#include <string.h>

struct dcp_input
{
    char const*                 filepath;
    FILE*                       stream;
    struct nmm_input*           nmm_input;
    uint32_t                    nprofiles;
    uint64_t*                   profile_offsets;
    struct dcp_metadata const** profile_metadatas;
    uint32_t                    curr_profile;

    bool closed;
};

int dcp_input_close(struct dcp_input* input)
{
    if (input->closed)
        return 0;

    int errno = 0;

    if (fclose(input->stream)) {
        imm_error("failed to close file %s", input->filepath);
        errno = 1;
    }

    input->closed = true;
    return errno;
}

struct dcp_input* dcp_input_create(char const* filepath)
{
    struct dcp_input* input = malloc(sizeof(*input));
    input->filepath = strdup(filepath);
    input->stream = fopen(filepath, "rb");
    input->profile_offsets = NULL;
    input->profile_metadatas = NULL;
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

    input->profile_metadatas = malloc(input->nprofiles * sizeof(*input->profile_metadatas));
    memset(input->profile_metadatas, 0, input->nprofiles);
    for (uint32_t i = 0; i < input->nprofiles; ++i) {

        input->profile_metadatas[i] = profile_metadata_read(input->stream);
        if (input->profile_metadatas[i] == NULL)
            goto err;
    }

    input->nmm_input = nmm_input_screate(filepath, input->stream);
    if (!input->nmm_input) {
        imm_error("could not nmm_input_screate");
        goto err;
    }

    input->curr_profile = 0;
    input->closed = false;

    return input;

err:
    if (input->filepath)
        free_c(input->filepath);

    if (input->stream)
        fclose(input->stream);

    if (input->profile_offsets)
        free_c(input->profile_offsets);

    if (input->profile_metadatas) {
        for (uint32_t i = 0; i < input->nprofiles; ++i) {
            if (input->profile_metadatas[i] == NULL)
                break;
            dcp_metadata_destroy(input->profile_metadatas[i]);
        }
        free_c(input->profile_metadatas);
    }

    if (input->nmm_input)
        nmm_input_destroy(input->nmm_input);

    free_c(input);
    return NULL;
}

int dcp_input_destroy(struct dcp_input* input)
{
    int errno = dcp_input_close(input);
    free_c(input->filepath);
    free_c(input->profile_offsets);
    for (uint32_t i = 0; i < input->nprofiles; ++i)
        dcp_metadata_destroy(input->profile_metadatas[i]);
    free_c(input->profile_metadatas);
    nmm_input_destroy(input->nmm_input);
    free_c(input);
    return errno;
}

bool dcp_input_end(struct dcp_input const* input) { return input->curr_profile >= input->nprofiles; }

struct dcp_profile const* dcp_input_read(struct dcp_input* input)
{
    if (dcp_input_end(input))
        return NULL;
    struct dcp_metadata const* mt = profile_metadata_clone(input->profile_metadatas[input->curr_profile]);
    return profile_create(input->curr_profile++, nmm_input_read(input->nmm_input), mt);
}

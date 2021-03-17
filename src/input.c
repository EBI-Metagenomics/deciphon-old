#include "dcp/dcp.h"
#include "metadata.h"
#include "nmm/nmm.h"
#include "profile.h"
#include "util.h"
#include <string.h>

struct dcp_input
{
    char const*                 filepath;
    FILE*                       stream;
    struct nmm_input*           nmm_input;
    uint32_t                    nprofiles;
    uint64_t const*             profile_offsets;
    struct dcp_metadata const** profile_metadatas;
    uint32_t                    profid;

    int64_t start;
    bool    closed;
};

int dcp_input_close(struct dcp_input* input)
{
    if (input->closed)
        return 0;

    int errno = 0;

    if (fclose(input->stream)) {
        error("failed to close file %s", input->filepath);
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
        error("could not open file %s for reading", filepath);
        goto err;
    }

    if (fread(&input->nprofiles, sizeof(input->nprofiles), 1, input->stream) < 1) {
        error("failed to read nprofiles");
        goto err;
    }

    uint64_t* profile_offsets = malloc(input->nprofiles * sizeof(*input->profile_offsets));
    input->profile_offsets = profile_offsets;
    if (fread(profile_offsets, sizeof(*profile_offsets), input->nprofiles, input->stream) < 1) {
        error("failed to read profile_offsets");
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
        error("could not nmm_input_screate");
        goto err;
    }

    input->profid = 0;
    input->closed = false;
    if ((input->start = imm_file_tell(input->stream)) < 0) {
        error("could not ftell %s", input->filepath);
        goto err;
    }

    return input;

err:
    if (input->filepath)
        free((void*)input->filepath);

    if (input->stream)
        fclose(input->stream);

    if (input->profile_offsets)
        free((void*)input->profile_offsets);

    if (input->profile_metadatas) {
        for (uint32_t i = 0; i < input->nprofiles; ++i) {
            if (input->profile_metadatas[i] == NULL)
                break;
            dcp_metadata_destroy(input->profile_metadatas[i]);
        }
        free(input->profile_metadatas);
    }

    if (input->nmm_input)
        nmm_input_destroy(input->nmm_input);

    free(input);
    return NULL;
}

int dcp_input_destroy(struct dcp_input* input)
{
    int errno = dcp_input_close(input);
    free((void*)input->filepath);
    free((void*)input->profile_offsets);
    for (uint32_t i = 0; i < input->nprofiles; ++i)
        dcp_metadata_destroy(input->profile_metadatas[i]);
    free(input->profile_metadatas);
    nmm_input_destroy(input->nmm_input);
    free(input);
    return errno;
}

bool dcp_input_end(struct dcp_input const* input) { return input->profid >= input->nprofiles; }

struct dcp_metadata const* dcp_input_metadata(struct dcp_input const* input, uint32_t profid)
{
    return input->profile_metadatas[profid];
}

uint32_t dcp_input_nprofiles(struct dcp_input const* input) { return input->nprofiles; }

struct dcp_profile const* dcp_input_read(struct dcp_input* input)
{
    if (dcp_input_end(input))
        return NULL;
    struct dcp_metadata const* mt = profile_metadata_clone(input->profile_metadatas[input->profid]);
    return profile_create(input->profid++, nmm_input_read(input->nmm_input), mt);
}

int dcp_input_reset(struct dcp_input* input)
{
    if (imm_file_seek(input->stream, input->start, SEEK_SET)) {
        error("could not fseek %s", input->filepath);
        return 1;
    }
    return 0;
}

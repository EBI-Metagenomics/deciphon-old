#include "deciphon/output.h"
#include "free.h"
#include "imm/imm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct dcp_output
{
    char const*        filepath;
    FILE*              stream;
    uint32_t           nmodels;
    uint64_t*          model_offsets;
    uint32_t           model_idx;
    struct nmm_output* nmm_output;
    char const*        nmm_filepath;
};

static char const* create_tmp_filepath(char const* filepath);

struct dcp_output* dcp_output_create(char const* filepath, uint32_t nmodels)
{
    char const* nmm_filepath = create_tmp_filepath(filepath);

    struct dcp_output* output = malloc(sizeof(*output));
    output->filepath = strdup(filepath);
    output->stream = NULL;
    output->nmodels = nmodels;
    output->model_offsets = malloc(nmodels * sizeof(*output->model_offsets));
    uint64_t start = sizeof(output->nmodels) + output->nmodels * sizeof(*output->model_offsets);
    for (uint32_t i = 0; i < nmodels; ++i)
        output->model_offsets[i] = start;
    output->model_idx = 0;
    output->nmm_output = nmm_output_create(nmm_filepath);
    output->nmm_filepath = nmm_filepath;
    if (!output->nmm_output) {
        imm_error("could not output create");
        goto ERROR;
    }

    output->stream = fopen(filepath, "wb");
    if (!output->stream) {
        imm_error("could not open file %s for writing", filepath);
        goto ERROR;
    }

    return output;

ERROR:
    free_c(output->filepath);
    if (output->stream)
        fclose(output->stream);
    free_c(output->model_offsets);
    if (output->nmm_output)
        nmm_output_destroy(output->nmm_output);
    free_c(output);
    free_c(nmm_filepath);
    return NULL;
}

int dcp_output_write(struct dcp_output* output, struct nmm_model const* model)
{
    IMM_BUG(output->model_idx == output->nmodels);
    output->model_offsets[output->model_idx++] += (uint64_t)nmm_output_ftell(output->nmm_output);
    return nmm_output_write(output->nmm_output, model);
}

int dcp_output_close(struct dcp_output* output)
{
    fclose(output->stream);
    return 0;
}

int dcp_output_destroy(struct dcp_output* output)
{
    int errno = 0;
    if (nmm_output_destroy(output->nmm_output)) {
        imm_error("could not output destroy");
        errno = 1;
        goto CLEANUP;
    }

    fwrite(&output->nmodels, sizeof(output->nmodels), 1, output->stream);
    for (uint32_t i = 0; i < output->nmodels; ++i) {
        fwrite(output->model_offsets + i, sizeof(*output->model_offsets), 1, output->stream);
    }

    FILE*         istream = fopen(output->nmm_filepath, "rb");
    char          buffer[100];
    unsigned long n = fread(buffer, sizeof(*buffer), 100, istream);
    while (n > 0) {
        fwrite(buffer, sizeof(*buffer), n, output->stream);
        n = fread(buffer, sizeof(*buffer), 100, istream);
    }

    if (fclose(output->stream)) {
        imm_error("failed to close file %s", output->filepath);
        errno = 1;
        goto CLEANUP;
    }

    fclose(istream);

CLEANUP:
    free_c(output->filepath);
    return errno;
}

static char const* create_tmp_filepath(char const* filepath)
{
    char* tmp_filepath = malloc(sizeof(char) * (strlen(filepath) + 5));
    memcpy(tmp_filepath, filepath, sizeof(char) * strlen(filepath));
    tmp_filepath[strlen(filepath) + 0] = '.';
    tmp_filepath[strlen(filepath) + 1] = 't';
    tmp_filepath[strlen(filepath) + 2] = 'm';
    tmp_filepath[strlen(filepath) + 3] = 'p';
    tmp_filepath[strlen(filepath) + 4] = '\0';
    return tmp_filepath;
}

#include "deciphon/deciphon.h"
#include "file.h"
#include "free.h"
#include "lib/c-list.h"
#include "nmm/nmm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct offset
{
    uint64_t value;
    CList    link;
};

struct dcp_output
{
    char const*        filepath;
    FILE*              stream;
    uint32_t           nprofiles;
    CList              profile_offsets;
    char const*        tmp_filepath;
    struct nmm_output* tmp_output;
};

static char const* create_tmp_filepath(char const* filepath);

int dcp_output_close(struct dcp_output* output)
{
    if (fclose(output->stream)) {
        imm_error("failed to close file %s", output->filepath);
        return 1;
    }
    return 0;
}

struct dcp_output* dcp_output_create(char const* filepath)
{
    struct dcp_output* output = malloc(sizeof(*output));
    output->filepath = strdup(filepath);
    output->stream = NULL;
    output->nprofiles = 0;
    c_list_init(&output->profile_offsets);
    output->tmp_filepath = create_tmp_filepath(filepath);
    output->tmp_output = NULL;

    if (!(output->stream = fopen(filepath, "wb"))) {
        imm_error("could not open file %s for writing", filepath);
        goto err;
    }

    if (!(output->tmp_output = nmm_output_create(output->tmp_filepath))) {
        imm_error("could not create output");
        goto err;
    }

    return output;

err:
    if (output->filepath)
        free_c(output->filepath);

    if (output->stream)
        fclose(output->stream);

    if (output->tmp_filepath)
        free_c(output->tmp_filepath);

    if (output->tmp_output)
        nmm_output_destroy(output->tmp_output);

    free_c(output);
    return NULL;
}

int dcp_output_destroy(struct dcp_output* output)
{
    int   errno = 0;
    FILE* tmp_stream = NULL;
    if (nmm_output_destroy(output->tmp_output)) {
        imm_error("could not destroy temporary output");
        errno = 1;
        goto cleanup;
    }

    if (fwrite(&output->nprofiles, sizeof(output->nprofiles), 1, output->stream) < 1) {
        imm_error("could not write nprofiles");
        errno = 1;
        goto cleanup;
    }

    struct offset* offset = NULL;
    uint64_t       start = sizeof(output->nprofiles) + output->nprofiles * sizeof(offset->value);
    c_list_for_each_entry (offset, &output->profile_offsets, link) {
        uint64_t v = start + offset->value;
        if (fwrite(&v, sizeof(v), 1, output->stream) < 1) {
            imm_error("could not write offset");
            errno = 1;
            goto cleanup;
        }
    }

    tmp_stream = fopen(output->tmp_filepath, "rb");
    if (!tmp_stream) {
        imm_error("failed to open %s", output->tmp_filepath);
        errno = 1;
        goto cleanup;
    }

    if (file_copy_content(output->stream, tmp_stream)) {
        imm_error("failed to copy file");
        errno = 1;
        goto cleanup;
    }

cleanup:
    if (output->stream && fclose(output->stream))
        imm_error("failed to close file %s", output->filepath);

    if (output->filepath)
        free_c(output->filepath);

    if (tmp_stream && fclose(tmp_stream))
        imm_error("failed to close file %s", output->tmp_filepath);

    if (output->tmp_filepath)
        free_c(output->tmp_filepath);

    struct offset* safe = NULL;
    c_list_for_each_entry_safe (offset, safe, &output->profile_offsets, link) {
        free_c(offset);
    }

    free_c(output);
    return errno;
}

int dcp_output_write(struct dcp_output* output, struct nmm_profile const* prof)
{
    struct offset* offset = malloc(sizeof(*offset));
    int64_t        v = nmm_output_ftell(output->tmp_output);
    if (v < 0) {
        imm_error("failed to ftell");
        free_c(offset);
        return 1;
    }
    offset->value = (uint64_t)v;
    c_list_link_tail(&output->profile_offsets, &offset->link);
    output->nprofiles++;
    return nmm_output_write(output->tmp_output, prof);
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

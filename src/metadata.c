#include "metadata.h"
#include "dcp/dcp.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

struct dcp_metadata
{
    char const* name;
    char const* acc;
};

struct dcp_metadata* dcp_metadata_create(char const* name, char const* acc)
{
    struct dcp_metadata* mt = malloc(sizeof(*mt));
    mt->name = strdup(name);
    mt->acc = strdup(acc);
    return mt;
}

void dcp_metadata_destroy(struct dcp_metadata const* mt)
{
    free((void*)mt->name);
    free((void*)mt->acc);
    free((void*)mt);
}

char const* dcp_metadata_acc(struct dcp_metadata const* mt) { return mt->acc; }

char const* dcp_metadata_name(struct dcp_metadata const* mt) { return mt->name; }

struct dcp_metadata const* profile_metadata_clone(struct dcp_metadata const* mt)
{
    struct dcp_metadata* clone = malloc(sizeof(*clone));
    clone->name = strdup(mt->name);
    clone->acc = strdup(mt->acc);
    return clone;
}

struct dcp_metadata const* profile_metadata_read(FILE* stream)
{
    char name[UINT8_MAX];
    char acc[UINT8_MAX];

    if (fread_string(stream, name, ARRAY_SIZE(name))) {
        error("failed to read name from metadata");
        return NULL;
    }

    if (fread_string(stream, acc, ARRAY_SIZE(acc))) {
        error("failed to read acc from metadata");
        return NULL;
    }

    return dcp_metadata_create(name, acc);
}

uint16_t profile_metadata_size(struct dcp_metadata const* mt)
{
    return (uint16_t)(strlen(mt->name) + strlen(mt->acc) + 2);
}

int profile_metadata_write(struct dcp_metadata const* mt, FILE* stream)
{
    int errno = fwrite(mt->name, sizeof(char), strlen(mt->name), stream) < strlen(mt->name);
    errno += fputc('\0', stream) != '\0';
    errno += fwrite(mt->acc, sizeof(char), strlen(mt->acc), stream) < strlen(mt->acc);
    errno += fputc('\0', stream) != '\0';

    if (errno) {
        error("failed to write metadata");
        return 1;
    }
    return 0;
}

#include "metadata.h"
#include "dcp/dcp.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

struct dcp_metadata const *profile_metadata_read(FILE *stream)
{
    char name[UINT8_MAX];
    char acc[UINT8_MAX];

    if (fread_string(stream, name, ARRAY_SIZE(name)))
    {
        error("failed to read name from metadata");
        return NULL;
    }

    if (fread_string(stream, acc, ARRAY_SIZE(acc)))
    {
        error("failed to read acc from metadata");
        return NULL;
    }

    return dcp_metadata_create(name, acc);
}

uint16_t profile_metadata_size(struct dcp_metadata const *mt)
{
    return (uint16_t)(strlen(mt->name) + strlen(mt->acc) + 2);
}

int profile_metadata_write(struct dcp_metadata const *mt, FILE *stream)
{
    int errno = fwrite(mt->name, sizeof(char), strlen(mt->name), stream) <
                strlen(mt->name);
    errno += fputc('\0', stream) != '\0';
    errno += fwrite(mt->acc, sizeof(char), strlen(mt->acc), stream) <
             strlen(mt->acc);
    errno += fputc('\0', stream) != '\0';

    if (errno)
    {
        error("failed to write metadata");
        return 1;
    }
    return 0;
}

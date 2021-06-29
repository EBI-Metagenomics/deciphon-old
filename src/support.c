#include "support.h"

#define BUFFSIZE (8 * 1024)

int fcopy(FILE *dst, FILE *src)
{
    char buffer[BUFFSIZE];
    size_t n = 0;
    while ((n = fread(buffer, sizeof(*buffer), BUFFSIZE, src)) > 0)
    {
        if (n < BUFFSIZE && ferror(src))
            return error(IMM_IOERROR, "failed to read file");

        if (fwrite(buffer, sizeof(*buffer), n, dst) < n)
            return error(IMM_IOERROR, "failed to write file");
    }
    if (ferror(src))
        return error(IMM_IOERROR, "failed to read file");

    return IMM_SUCCESS;
}

#include "fcopy.h"
#include "error.h"

#define BUFFSIZE (8 * 1024)

enum dcp_rc fcopy(FILE *restrict dst, FILE *restrict src)
{
    char buffer[BUFFSIZE];
    size_t n = 0;
    while ((n = fread(buffer, sizeof(*buffer), BUFFSIZE, src)) > 0)
    {
        if (n < BUFFSIZE && ferror(src))
            return error(DCP_IOERROR, "failed to read file");

        if (fwrite(buffer, sizeof(*buffer), n, dst) < n)
            return error(DCP_IOERROR, "failed to write file");
    }
    if (ferror(src)) return error(DCP_IOERROR, "failed to read file");

    return DCP_SUCCESS;
}

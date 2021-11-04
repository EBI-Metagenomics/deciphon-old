#include "xfile.h"
#include "error.h"
#include <assert.h>
#include <unistd.h>

#ifndef __USE_XOPEN_EXTENDED
/* To make mkstemp available. */
#define __USE_XOPEN_EXTENDED
#endif
#include <stdlib.h>

#define BUFFSIZE (8 * 1024)

enum dcp_rc xfile_copy(FILE *restrict dst, FILE *restrict src)
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

    return DCP_DONE;
}

bool xfile_is_readable(char const filepath[static 1])
{
    FILE *file = NULL;
    if ((file = fopen(filepath, "r")))
    {
        fclose(file);
        return true;
    }
    return false;
}

enum dcp_rc xfile_mktemp(char filepath[static 1])
{
    if (mkstemp(filepath) == -1) return error(DCP_IOERROR, "mkstemp failed");
    return DCP_DONE;
}

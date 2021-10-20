#include "dcp_file.h"
#include "error.h"

#define BUFFSIZE (8 * 1024)

enum dcp_rc file_copy(FILE *restrict dst, FILE *restrict src)
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

enum dcp_rc file_empty(char const *filepath)
{
    if (!fopen(filepath, "wb"))
        return error(DCP_IOERROR, "failed to empty a file");
    return DCP_SUCCESS;
}

bool file_readable(char const *filepath)
{
    FILE *file = NULL;
    if ((file = fopen(filepath, "r")))
    {
        fclose(file);
        return true;
    }
    return false;
}

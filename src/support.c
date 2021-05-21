#include "support.h"
#include "rand.h"

char const *tempfile(char const *filepath)
{
    size_t n = strlen(filepath);
    char *newfp = xmalloc(sizeof(char) * (n + 8));
    xmemcpy(newfp, filepath, sizeof(char) * n);
    newfp[n] = '-';
    rand_string(6, newfp + n + 1);
    newfp[n + 7] = '\0';
    return newfp;
}

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

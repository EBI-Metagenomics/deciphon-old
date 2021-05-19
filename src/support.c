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

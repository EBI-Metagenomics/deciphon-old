#include "helper.h"
#include <stdlib.h>

char *append_char(size_t n, char *dst, char c)
{
    char *t = realloc(dst, n + 1);
    if (!t) exit(EXIT_FAILURE);
    t[n] = c;
    return t;
}

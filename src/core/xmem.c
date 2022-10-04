#include "core/xmem.h"
#include "core/logy.h"
#include <stdlib.h>
#include <string.h>

void *xmalloc(size_t size)
{
    void *m = malloc(size);
    if (!m) fatal("malloc");
    return m;
}

void *xmemdup(void const *mem, size_t size)
{
    void *out = xmalloc(size);
    memcpy(out, mem, size);
    return out;
}

void *xrealloc(void *mem, size_t size)
{
    void *m = realloc(mem, size);
    if (!m) fatal("realloc");
    return m;
}

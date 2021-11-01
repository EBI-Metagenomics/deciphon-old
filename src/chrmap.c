#include "chrmap.h"
#include "bits.h"
#include "dcp/strlcpy.h"
#include "ilog2.h"
#include <stdlib.h>
#include <string.h>

#define LONG_START(bit) (bit / (sizeof(long) * BITS_PER_BYTE))
#define BIT_START(bit) (bit % (sizeof(long) * BITS_PER_BYTE))

void chrmap_init(struct chrmap *x, char map[CHRMAP_SIZE])
{
    dcp_strlcpy(x->map, map, CHRMAP_SIZE);
    x->bits = ilog2((uint32_t)strlen(map));
    x->chars = 0;
    x->data = NULL;
}

char chrmap_get(struct chrmap const *x, unsigned long pos)
{
    unsigned val = 0;
    for (unsigned i = 0; i < x->bits; ++i)
    {
        unsigned long j = pos + (unsigned long)i;
        if (bits_get(x->data + LONG_START(j), BIT_START(j)))
            bits_set(&val, i);
        else
            bits_clr(&val, i);
    }
    return val;
}

struct chrmap *chrmap_realloc(struct chrmap *x, unsigned long chars)
{
    return realloc(x, sizeof(long) * BITS_TO_LONGS(x->bits * chars));
}

void chrmap_set(struct chrmap *x, char val, unsigned long pos)
{
    for (unsigned i = 0; i < len; ++i)
    {
        unsigned long j = start + (unsigned long)i;
        if (bits_get(&val, i))
            bits_set(x + LONG_START(j), BIT_START(j));
        else
            bits_clr(x + LONG_START(j), BIT_START(j));
    }
}

void chrmap_del(struct chrmap *x) { free(x->data); }

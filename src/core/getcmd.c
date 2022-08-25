#include "deciphon/core/getcmd.h"
#include "deciphon/core/to.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define DELIM_CHARS " \t,"

bool getcmd_parse(struct getcmd *gc, char *str)
{
    char *p = str;
    gc->argc = 0;
    gc->argv[gc->argc++] = p;

    if (!(p = strpbrk(str, DELIM_CHARS))) return true;
    char const delim = *p;

    while (p && gc->argc < GETCMD_ARGC_MAX)
    {
        *p++ = 0;
        gc->argv[gc->argc++] = p;
        p = strchr(p, delim);
    }

    return !p;
}

int64_t getcmd_i64(struct getcmd const *gc, int i)
{
    int64_t i64 = 0;
    (void)to_int64(gc->argv[i], &i64);
    return i64;
}

bool getcmd_check(struct getcmd const *gc, char const *fmt)
{
    if (strlen(fmt) > GETCMD_ARGC_MAX) return false;

    int const n = (int)strlen(fmt);
    if (n != gc->argc) return false;

    for (int i = 0; i < n; ++i)
    {
        if (fmt[i] == 's') continue;

        int64_t i64 = 0;
        if (fmt[i] == 'i' && !to_int64(gc->argv[i], &i64)) return false;
    }

    return true;
}

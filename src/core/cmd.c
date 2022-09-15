#include "core/cmd.h"
#include "core/to.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define DELIM_CHARS " \t,"

bool cmd_parse(struct cmd *c, char *str)
{
    char *p = str;
    c->argc = 0;
    c->argv[c->argc++] = p;

    if (!(p = strpbrk(str, DELIM_CHARS))) return true;
    char const delim = *p;

    while (p && c->argc < CMD_ARGC_MAX)
    {
        *p++ = 0;
        c->argv[c->argc++] = p;
        p = strchr(p, delim);
    }

    return !p;
}

int64_t cmd_get_i64(struct cmd const *gc, int i)
{
    int64_t i64 = 0;
    (void)to_int64(gc->argv[i], &i64);
    return i64;
}

bool cmd_check(struct cmd const *gc, char const *fmt)
{
    if (strlen(fmt) > CMD_ARGC_MAX) return false;

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

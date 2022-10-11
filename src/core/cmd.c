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

    if (!(p = strpbrk(str, DELIM_CHARS))) goto cleanup;

    char const delim = *p;

    while (p && c->argc + 1 < CMD_ARGV_SIZE)
    {
        *p++ = 0;
        c->argv[c->argc++] = p;
        p = strchr(p, delim);
    }

cleanup:
    c->argv[c->argc] = NULL;
    return !p;
}

void cmd_shift(struct cmd *c)
{
    for (int i = 0; i < c->argc; ++i)
        c->argv[i] = c->argv[i + 1];
    c->argc--;
}

char const *cmd_get(struct cmd const *gc, int i) { return gc->argv[i]; }

int64_t cmd_as_i64(struct cmd const *gc, int i)
{
    int64_t i64 = 0;
    (void)to_int64(gc->argv[i], &i64);
    return i64;
}

bool cmd_check(struct cmd const *gc, char const *fmt)
{
    int n = (int)strcspn(fmt, "*");
    if (n > CMD_ARGV_SIZE + 1) return false;

    int m = (int)strlen(fmt);
    if (n == m && n != gc->argc) return false;
    if (n < m && n > gc->argc) return false;

    for (int i = 0; i < n; ++i)
    {
        if (fmt[i] == 's') continue;

        int64_t i64 = 0;
        if (fmt[i] == 'i' && !to_int64(gc->argv[i], &i64)) return false;
    }

    return true;
}

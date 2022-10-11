#include "core/cmd.h"
#include "core/to.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define DELIM_CHARS " \t,"

bool cmd_parse(struct cmd *c, char *str)
{
    c->delim = '\t';
    char *p = str;
    c->argc = 0;
    c->argv[c->argc++] = p;

    if (!(p = strpbrk(str, DELIM_CHARS))) goto cleanup;

    c->delim = *p;

    while (p && c->argc + 1 < CMD_ARGV_SIZE)
    {
        *p++ = '\0';
        c->argv[c->argc++] = p;
        p = strchr(p, c->delim);
    }

cleanup:
    c->argv[c->argc] = NULL;
    return !p;
}

char *cmd_unparse(struct cmd *c)
{
    if (c->argc <= 0) return NULL;
    for (int i = 0; i < c->argc - 1; ++i)
    {
        size_t n = strlen(c->argv[i]);
        c->argv[i][n] = c->delim;
    }
    return c->argv[0];
}

char const *cmd_shift(struct cmd *c)
{
    char const *argv0 = c->argv[0];
    for (int i = 0; i < c->argc; ++i)
        c->argv[i] = c->argv[i + 1];
    c->argc--;
    return argv0;
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

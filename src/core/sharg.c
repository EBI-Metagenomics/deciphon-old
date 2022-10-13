#include "core/sharg.h"
#include "core/is.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define DELIM_CHARS " \t,"

void sharg_init(struct sharg *sharg)
{
    sharg->argc = 0;
    sharg->argv[0] = NULL;
    sharg->delim = '\t';
    sharg->buff[0] = '\0';
}

bool sharg_parse(struct sharg *sharg, char const *str)
{
    if (strlen(str) >= SHARG_BUFF_SIZE) return 0;
    memcpy(sharg->buff, str, strlen(str) + 1);
    sharg->delim = '\t';
    char *p = sharg->buff;
    sharg->argc = 0;
    sharg->argv[sharg->argc++] = p;

    if (!(p = strpbrk(sharg->buff, DELIM_CHARS))) goto cleanup;

    sharg->delim = *p;

    while (p && sharg->argc + 1 < SHARG_ARGV_SIZE)
    {
        *p++ = '\0';
        sharg->argv[sharg->argc++] = p;
        p = strchr(p, sharg->delim);
    }

cleanup:
    sharg->argv[sharg->argc] = NULL;
    return !p;
}

char sharg_delim(struct sharg const *sharg) { return sharg->delim; }

int sharg_argc(struct sharg const *sharg) { return sharg->argc; }

char **sharg_argv(struct sharg *sharg) { return sharg->argv; }

char const *sharg_shift(struct sharg *sharg)
{
    if (sharg->argc == 0) return NULL;
    char const *argv0 = sharg->argv[0];
    memmove(sharg->argv, sharg->argv + 1, sizeof(sharg->argv[0]) * sharg->argc);
    sharg->argc--;
    return argv0;
}

void sharg_append(struct sharg *sharg, char const *str)
{
    int offset = 0;
    for (int i = 0; i < sharg->argc; ++i)
        offset += (int)strlen(sharg->argv[i]) + 1;

    assert(offset < SHARG_BUFF_SIZE);
    sharg->argv[sharg->argc] = sharg->buff + offset;
    assert(offset + strlen(str) + 1 < SHARG_BUFF_SIZE);
    memcpy(sharg->argv[sharg->argc], str, strlen(str) + 1);

    assert(sharg->argc + 1 < SHARG_ARGV_SIZE);
    sharg->argv[++sharg->argc] = NULL;
}

bool sharg_replace(struct sharg *sharg, char const *tag, char const *value)
{
    for (int i = 0; i < sharg->argc; ++i)
    {
        if (!strcmp(sharg->argv[i], tag))
        {
            char *pos = sharg->argv[i];
            char *src = pos + strlen(pos) + 1;
            char *dst = src + strlen(value) - strlen(tag);
            char *end = sharg->argv[sharg->argc - 1];
            size_t count = end - src + strlen(end) + 1;
            if (dst + count >= sharg->buff + SHARG_BUFF_SIZE) return false;
            memmove(dst, src, count);
            memcpy(pos, value, strlen(value) + 1);

            int shift = (int)(dst - src);
            for (int j = i + 1; j < sharg->argc; ++j)
                sharg->argv[j] += shift;

            return true;
        }
    }
    return true;
}

bool sharg_check(struct sharg const *sharg, char const *fmt)
{
    int n = (int)strcspn(fmt, "*");
    if (n > SHARG_ARGV_SIZE + 1) return false;

    int m = (int)strlen(fmt);
    if (n == m && n != sharg->argc) return false;
    if (n < m && n > sharg->argc) return false;

    for (int i = 0; i < n; ++i)
    {
        if (fmt[i] == 's') continue;

        if (fmt[i] == 'i' && !is_int64(sharg->argv[i])) return false;
    }

    return true;
}

char *sharg_unparse(struct sharg *sharg)
{
    if (sharg->argc <= 0) return NULL;
    for (int i = 0; i < sharg->argc - 1; ++i)
    {
        size_t n = strlen(sharg->argv[i]);
        sharg->argv[i][n] = sharg->delim;
    }
    return sharg->argv[0];
}

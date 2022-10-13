#include "core/sharg.h"
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

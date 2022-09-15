#ifndef CORE_GETCMD_H
#define CORE_GETCMD_H

#include <stdbool.h>
#include <stdint.h>

enum
{
    GETCMD_ARGC_MAX = 5,
};

struct getcmd
{
    int argc;
    char *argv[GETCMD_ARGC_MAX];
};

typedef char const *getcmd_fn_t(struct getcmd const *);

bool getcmd_parse(struct getcmd *gc, char *str);
int64_t getcmd_i64(struct getcmd const *gc, int i);
bool getcmd_check(struct getcmd const *gc, char const *fmt);

#endif

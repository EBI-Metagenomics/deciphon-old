#ifndef CORE_CMD_H
#define CORE_CMD_H

#include <stdbool.h>
#include <stdint.h>

enum
{
    CMD_ARGC_MAX = 5,
};

struct cmd
{
    int argc;
    char *argv[CMD_ARGC_MAX];
};

typedef char const *cmd_fn_t(struct cmd const *);

bool cmd_parse(struct cmd *, char *str);
int64_t cmd_get_i64(struct cmd const *, int i);
bool cmd_check(struct cmd const *, char const *fmt);

#endif

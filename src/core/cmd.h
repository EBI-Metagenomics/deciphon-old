#ifndef CORE_CMD_H
#define CORE_CMD_H

#include <stdbool.h>
#include <stdint.h>

enum
{
    CMD_ARGV_SIZE = 12,
};

struct cmd
{
    int argc;
    char *argv[CMD_ARGV_SIZE];
};

typedef char const *(cmd_fn_t)(struct cmd *);

bool cmd_parse(struct cmd *, char *str);
void cmd_shift(struct cmd *);
char const *cmd_get(struct cmd const *, int i);
int64_t cmd_as_i64(struct cmd const *, int i);
bool cmd_check(struct cmd const *, char const *fmt);

#endif

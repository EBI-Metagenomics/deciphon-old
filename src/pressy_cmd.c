#include "pressy_cmd.h"
#include "core/pp.h"
#include <string.h>

static char buffer[6 * 1024 * 1024] = {0};

static cmd_fn_t *pressy_cmds[] = {
#define X(_, A) &A,
    PRESSY_CMD_MAP(X)
#undef X
};

static enum pressy_cmd parse(char const *cmd)
{
#define X(A, _)                                                                \
    if (!strcmp(cmd, STRINGIFY(A))) return PRESSY_CMD_##A;
    PRESSY_CMD_MAP(X)
#undef X
    return PRESSY_CMD_INVALID;
}

cmd_fn_t *pressy_cmd(char const *cmd) { return pressy_cmds[parse(cmd)]; }

char const *pressy_cmd_invalid(struct cmd const *cmd) {}

char const *pressy_cmd_press(struct cmd const *cmd) {}

char const *pressy_cmd_state(struct cmd const *cmd) {}

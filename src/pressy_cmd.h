#ifndef PRESSY_CMD_H
#define PRESSY_CMD_H

#include "core/cmd.h"

#define PRESSY_CMD_MAP(X)                                                      \
    X(INVALID, pressy_cmd_invalid)                                             \
    X(PRESS, pressy_cmd_press)                                                 \
    X(CANCEL, pressy_cmd_cancel)                                               \
    X(STATE, pressy_cmd_state)                                                 \
    X(PROGRESS, pressy_cmd_progress)

enum pressy_cmd
{
#define X(A, _) PRESSY_CMD_##A,
    PRESSY_CMD_MAP(X)
#undef X
};

struct uv_loop_s;

cmd_fn_t *pressy_cmd(char const *cmd);

#define X(_, X) char const *X(struct cmd const *);
PRESSY_CMD_MAP(X)
#undef X

#endif

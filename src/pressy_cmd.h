#ifndef PRESSY_CMD_H
#define PRESSY_CMD_H

#include "core/cmd.h"

#define PRESSY_CMD_MAP(X)                                                      \
    X(INVALID, invalid, "")                                                    \
    X(HELP, help, "")                                                          \
    X(PRESS, press, "HMM_FILE")                                                \
    X(CANCEL, cancel, "")                                                      \
    X(STATE, state, "")                                                        \
    X(PROGRESS, progress, "")

enum pressy_cmd
{
#define X(A, _1, _2) PRESSY_CMD_##A,
    PRESSY_CMD_MAP(X)
#undef X
};

cmd_fn_t *pressy_cmd(char const *cmd);

#define X(_1, A, _2) char const *pressy_cmd_##A(struct cmd const *);
PRESSY_CMD_MAP(X)
#undef X

#endif

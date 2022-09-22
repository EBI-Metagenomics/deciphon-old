#ifndef SCANNY_CMD_H
#define SCANNY_CMD_H

#include "core/cmd.h"

#define SCANNY_CMD_MAP(X)                                                      \
    X(INVALID, scanny_cmd_invalid)                                             \
    X(SCAN, scanny_cmd_scan)                                                   \
    X(CANCEL, scanny_cmd_cancel)                                               \
    X(STATE, scanny_cmd_state)                                                 \
    X(PROGRESS, scanny_cmd_progress)

enum scanny_cmd
{
#define X(A, _) SCANNY_CMD_##A,
    SCANNY_CMD_MAP(X)
#undef X
};

cmd_fn_t *scanny_cmd(char const *cmd);

#define X(_, X) char const *X(struct cmd const *);
SCANNY_CMD_MAP(X)
#undef X

#endif

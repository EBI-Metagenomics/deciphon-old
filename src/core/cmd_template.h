#ifndef CORE_CMD_TEMPLATE_H
#define CORE_CMD_TEMPLATE_H

#ifdef CMD_TEMPLATE_ENABLE

#include "core/pp.h"
#include <string.h>

enum
{
#define X(A, _1, _2) FN_##A,
    CMD_MAP(X)
#undef X
};

#define X(_1, A, _2) static char const *fn_##A(struct cmd const *);
CMD_MAP(X)
#undef X

static cmd_fn_t *cmd_fns[] = {
#define X(_1, A, _2) &fn_##A,
    CMD_MAP(X)
#undef X
};

static int cmd_fn_idx(char const *cmd)
{
#define X(A, B, _)                                                             \
    if (!strcmp(cmd, STRINGIFY(B))) return FN_##A;
    CMD_MAP(X)
#undef X
    return 0;
}

cmd_fn_t *cmd_fn(char const *cmd) { return cmd_fns[cmd_fn_idx(cmd)]; }

#endif

#endif

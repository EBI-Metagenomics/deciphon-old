struct msg;
typedef void cmd_fn_t(struct msg *);

#ifdef COMMAND_TEMPLATE_DEF

#include "core/logy.h"
#include "core/pp.h"
#include <stddef.h>
#include <string.h>

enum
{
#define X(A, _1, _2) FN_##A,
    CMD_MAP(X)
#undef X
};

#define X(_1, A, _2) static void fn_##A(struct msg *);
CMD_MAP(X)
#undef X

static cmd_fn_t *cmd_fns[] = {
#define X(_1, A, _2) &fn_##A,
    CMD_MAP(X)
#undef X
};

static int cmd_fn_idx(char const *cmd_string)
{
#define X(A, B, _)                                                             \
    if (!strcmp(cmd_string, STRINGIFY(B))) return FN_##A;
    CMD_MAP(X)
#undef X
    return -1;
}

cmd_fn_t *cmd_get_fn(char const *cmd_string)
{
    int idx = cmd_fn_idx(cmd_string);
    if (idx < 0)
    {
        einval("unrecognized command: %s", cmd_string);
        return NULL;
    }
    return cmd_fns[idx];
}

#endif

#ifdef COMMAND_TEMPLATE_DECL
cmd_fn_t *cmd_get_fn(char const *cmd_string);
#endif

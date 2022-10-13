#ifndef CORE_MSG_TEMPLATE_H
#define CORE_MSG_TEMPLATE_H

#ifdef MSG_TEMPLATE_ENABLE

#include "core/pp.h"
#include <string.h>

enum
{
#define X(A, _1, _2) FN_##A,
    MSG_MAP(X)
#undef X
};

#define X(_1, A, _2) static char const *fn_##A(struct msg *);
MSG_MAP(X)
#undef X

static msg_fn_t *msg_fns[] = {
#define X(_1, A, _2) &fn_##A,
    MSG_MAP(X)
#undef X
};

static int msg_fn_idx(char const *msg)
{
#define X(A, B, _)                                                             \
    if (!strcmp(msg, STRINGIFY(B))) return FN_##A;
    MSG_MAP(X)
#undef X
    return 0;
}

msg_fn_t *msg_fn(char const *msg) { return msg_fns[msg_fn_idx(msg)]; }

#endif

#endif

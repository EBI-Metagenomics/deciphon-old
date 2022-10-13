#include "msg.h"
#include "core/file.h"
#include "core/logy.h"
#include "core/sched_dump.h"
#include "decy.h"
#include "session.h"
#include "strings.h"
#include "xfile.h"
#include <string.h>

#define MSG_MAP(X)                                                             \
    X(INVALID, invalid, "")                                                    \
    X(HELP, help, "")                                                          \
    X(PRESSY, pressy, "PRESSY_COMMAND [...]")                                  \
    X(SCANNY, scanny, "SCANNY_COMMAND [...]")                                  \
    X(SCHEDY, schedy, "SCHEDY_COMMAND [...]")

#define MSG_TEMPLATE_ENABLE
#include "core/msg_template.h"
#undef MSG_TEMPLATE_ENABLE

static char const *fn_invalid(struct msg *msg)
{
    if (!sharg_check(&msg->cmd, "s")) return eparse(FAIL_PARSE), FAIL;
    return eparse("invalid command"), FAIL;
}

static char const *fn_help(struct msg *msg)
{
    if (!sharg_check(&msg->cmd, "s")) return eparse(FAIL_PARSE), FAIL;

    static char help_table[1024] = {0};
    char *p = help_table;
    p += sprintf(p, "Commands:");

#define X(_, A, B)                                                             \
    if (strcmp(STRINGIFY(A), "invalid"))                                       \
        p += sprintf(p, "\n  %-11s %s", STRINGIFY(A), B);
    MSG_MAP(X);
#undef X

    return help_table;
}

static char const *fn_pressy(struct msg *msg)
{
    if (!sharg_check(&msg->cmd, "ss*")) return eparse(FAIL_PARSE), FAIL;
    return session_forward_msg(sharg_shift(&msg->cmd), msg);
}

static char const *fn_scanny(struct msg *msg)
{
    if (!sharg_check(&msg->cmd, "ss*")) return eparse(FAIL_PARSE), FAIL;
    return session_forward_msg(sharg_shift(&msg->cmd), msg);
}

static char const *fn_schedy(struct msg *msg)
{
    if (!sharg_check(&msg->cmd, "ss*")) return eparse(FAIL_PARSE), FAIL;
    return session_forward_msg(sharg_shift(&msg->cmd), msg);
}

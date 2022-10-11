#include "decy/cmd.h"
#include "core/api.h"
#include "core/file.h"
#include "core/logy.h"
#include "core/sched_dump.h"
#include "decy/decy.h"
#include "decy/session.h"
#include "schedy/strings.h"
#include "xfile.h"
#include <string.h>

#define CMD_MAP(X)                                                             \
    X(INVALID, invalid, "")                                                    \
    X(HELP, help, "")                                                          \
    X(PRESSY, pressy, "PRESSY_COMMAND [...]")                                  \
    X(SCANNY, scanny, "SCANNY_COMMAND [...]")                                  \
    X(SCHEDY, schedy, "SCHEDY_COMMAND [...]")

#define CMD_TEMPLATE_ENABLE
#include "core/cmd_template.h"
#undef CMD_TEMPLATE_ENABLE

static char const *fn_invalid(struct cmd *cmd)
{
    if (!cmd_check(cmd, "s")) return eparse(FAIL_PARSE), FAIL;
    return eparse("invalid command"), FAIL;
}

static char const *fn_help(struct cmd *cmd)
{
    if (!cmd_check(cmd, "s")) return eparse(FAIL_PARSE), FAIL;

    static char help_table[1024] = {0};
    char *p = help_table;
    p += sprintf(p, "Commands:");

#define X(_, A, B)                                                             \
    if (strcmp(STRINGIFY(A), "invalid"))                                       \
        p += sprintf(p, "\n  %-11s %s", STRINGIFY(A), B);
    CMD_MAP(X);
#undef X

    return help_table;
}

static char const *fn_pressy(struct cmd *cmd)
{
    if (!cmd_check(cmd, "ss*")) return eparse(FAIL_PARSE), FAIL;
    return session_forward_command(cmd_shift(cmd), cmd);
}

static char const *fn_scanny(struct cmd *cmd)
{
    if (!cmd_check(cmd, "ss*")) return eparse(FAIL_PARSE), FAIL;
    return session_forward_command(cmd_shift(cmd), cmd);
}

static char const *fn_schedy(struct cmd *cmd)
{
    if (!cmd_check(cmd, "ss*")) return eparse(FAIL_PARSE), FAIL;
    return session_forward_command(cmd_shift(cmd), cmd);
}

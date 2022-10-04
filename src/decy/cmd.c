#include "decy/cmd.h"
#include "core/api.h"
#include "core/file.h"
#include "core/logy.h"
#include "core/sched_dump.h"
#include "decy/decy.h"
#include "schedy/strings.h"
#include "xfile.h"
#include <string.h>

#define CMD_MAP(X)                                                             \
    X(INVALID, invalid, "")                                                    \
    X(HELP, help, "")                                                          \
    X(TARGET, target, "PROCESS")

#define CMD_TEMPLATE_ENABLE
#include "core/cmd_template.h"
#undef CMD_TEMPLATE_ENABLE

static char const *fn_invalid(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "s")) return eparse(FAIL_PARSE), FAIL;
    return eparse("invalid command"), FAIL;
}

static char const *fn_help(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "s")) return eparse(FAIL_PARSE), FAIL;

    static char help_table[1024] = {0};
    char *p = help_table;
    p += sprintf(p, "Commands:");

#define X(_, A, B)                                                             \
    if (strcmp(STRINGIFY(A), "invalid"))                                       \
        p += sprintf(p, "\n  %-22s %s", STRINGIFY(A), B);
    CMD_MAP(X);
#undef X

    return help_table;
}

static char const *fn_target(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "ss")) return eparse(FAIL_PARSE), FAIL;
    if (!strcmp(cmd_get(cmd, 1), "decy"))
    {
        target = TARGET_DECY;
        return OK;
    }
    if (!strcmp(cmd_get(cmd, 1), "pressy"))
    {
        target = TARGET_PRESSY;
        return OK;
    }
    return FAIL;
}

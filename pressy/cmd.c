#include "cmd.h"
#include "core/logy.h"
#include "session.h"
#include "strings.h"
#include <stdio.h>

#define CMD_MAP(X)                                                             \
    X(INVALID, invalid, "")                                                    \
    X(HELP, help, "")                                                          \
    X(PRESS, press, "HMM_FILE")                                                \
    X(CANCEL, cancel, "")                                                      \
    X(STATE, state, "")                                                        \
    X(PROGRESS, progress, "")

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

    static char help_table[512] = {0};
    char *p = help_table;
    p += sprintf(p, "Commands:");

#define X(_, A, B)                                                             \
    if (strcmp(STRINGIFY(A), "invalid"))                                       \
        p += sprintf(p, "\n  %-11s %s", STRINGIFY(A), B);
    CMD_MAP(X);
#undef X

    return help_table;
}

static char const *fn_press(struct cmd *cmd)
{
    if (!cmd_check(cmd, "ss")) return eparse(FAIL_PARSE), FAIL;
    if (session_is_running()) return BUSY;
    return session_start(cmd_get(cmd, 1)) ? OK : FAIL;
}

static char const *fn_cancel(struct cmd *cmd)
{
    if (!cmd_check(cmd, "s")) return eparse(FAIL_PARSE), FAIL;
    if (!session_is_running()) return DONE;
    return session_cancel() ? OK : FAIL;
}

static char const *fn_state(struct cmd *cmd)
{
    if (!cmd_check(cmd, "s")) return eparse(FAIL_PARSE), FAIL;
    return session_state_string();
}

static char const *fn_progress(struct cmd *cmd)
{
    if (!cmd_check(cmd, "s")) return eparse(FAIL_PARSE), FAIL;

    static char progress[5] = "100%";
    if (session_is_done()) return progress;

    if (session_is_running())
    {
        sprintf(progress, "%u%%", session_progress());
        return progress;
    }

    return FAIL;
}

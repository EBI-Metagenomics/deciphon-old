#include "msg.h"
#include "core/logy.h"
#include "core/service_strings.h"
#include "core/strings.h"
#include "session.h"
#include "zc.h"
#include <stdio.h>

#define MSG_MAP(X)                                                             \
    X(INVALID, invalid, "")                                                    \
    X(HELP, help, "")                                                          \
    X(PRESS, press, "HMM_FILE")                                                \
    X(CANCEL, cancel, "")                                                      \
    X(STATE, state, "")                                                        \
    X(PROGRESS, progress, "")

#define MSG_TEMPLATE_ENABLE
#include "core/msg_template.h"
#undef MSG_TEMPLATE_ENABLE

static char const *fn_invalid(struct msg *msg)
{
    UNUSED(msg);
    return eparse("invalid command"), NULL;
}

static char const *fn_help(struct msg *msg)
{
    UNUSED(msg);

    static char help_table[512] = {0};
    char *p = help_table;
    p += sprintf(p, "Commands:");

#define X(_, A, B)                                                             \
    if (strcmp(STRINGIFY(A), "invalid"))                                       \
        p += sprintf(p, "\n  %-11s %s", STRINGIFY(A), B);
    MSG_MAP(X);
#undef X

    return help_table;
}

static char const *fn_press(struct msg *msg)
{
    char const *ans = NULL;
    if (!sharg_check(&msg->cmd, "ss"))
    {
        eparse(FAIL_PARSE_CMD);
        ans = FAIL;
    }
    else if (session_is_running())
        ans = BUSY;
    else if (session_start(msg->cmd.argv[1]))
        ans = OK;
    else
        ans = FAIL;
    sharg_replace(&msg->echo, "$1", ans);
    return sharg_unparse(&msg->echo);
}

static char const *fn_cancel(struct msg *msg)
{
    char const *ans = NULL;
    if (!sharg_check(&msg->cmd, "s"))
    {
        eparse(FAIL_PARSE_CMD);
        ans = FAIL;
    }
    else if (!session_is_running() || session_cancel())
        ans = OK;
    else
        ans = FAIL;
    sharg_replace(&msg->echo, "$1", ans);
    return sharg_unparse(&msg->echo);
}

static char const *fn_state(struct msg *msg)
{
    char const *ans = NULL;
    char const *state = NULL;
    if (!sharg_check(&msg->cmd, "s"))
    {
        eparse(FAIL_PARSE_CMD);
        ans = FAIL;
        state = "";
    }
    else
    {
        ans = OK;
        state = session_state_string();
    }
    sharg_replace(&msg->echo, "$1", ans);
    sharg_replace(&msg->echo, "$2", state);
    return sharg_unparse(&msg->echo);
}

static char const *fn_progress(struct msg *msg)
{
    char const *ans = NULL;
    char progress[16] = {0};
    if (!sharg_check(&msg->cmd, "s"))
    {
        eparse(FAIL_PARSE_CMD);
        ans = FAIL;
    }
    else if (session_is_done())
    {
        ans = OK;
        zc_strlcpy(progress, "100%", sizeof progress);
    }
    else if (session_is_running())
    {
        ans = OK;
        sprintf(progress, "%u%%", session_progress());
    }
    else
    {
        ans = FAIL;
    }
    sharg_replace(&msg->echo, "$1", ans);
    sharg_replace(&msg->echo, "$2", progress);
    return sharg_unparse(&msg->echo);
}

#include "msg.h"
#include "core/logy.h"
#include "core/service_strings.h"
#include "core/strings.h"
#include "pressy.h"
#include "session.h"
#include "zc.h"
#include <stdio.h>

#define MSG_MAP(X)                                                             \
    X(INVALID, invalid, "")                                                    \
    X(HELP, help, "")                                                          \
    X(ECHO, echo, "[...]")                                                     \
    X(PRESS, press, "HMM_FILE")                                                \
    X(CANCEL, cancel, "")                                                      \
    X(STATE, state, "")                                                        \
    X(PROGRESS, progress, "")

#define MSG_TEMPLATE_ENABLE
#include "core/msg_template.h"
#undef MSG_TEMPLATE_ENABLE

static void fn_invalid(struct msg *msg)
{
    warn("invalid command: %s", msg->cmd.argv[0]);
    output_put(&output, msg_unparse(msg));
}

static void fn_help(struct msg *msg)
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

    output_put(&output, help_table);
}

static void fn_echo(struct msg *msg) { output_put(&output, msg_unparse(msg)); }

#define eparse_cleanup()                                                       \
    do                                                                         \
    {                                                                          \
        eparse(INVALID_ARGS);                                                  \
        goto cleanup;                                                          \
    } while (0);

static void fn_press(struct msg *msg)
{
    char const *ans = FAIL;
    if (!sharg_check(&msg->cmd, "ss")) eparse_cleanup();
    if (session_is_running())
        ans = BUSY;
    else if (session_start(msg->cmd.argv[1]))
        ans = OK;
    else
        ans = FAIL;

cleanup:
    sharg_replace(&msg->ctx, "{1}", ans);
    output_put(&output, sharg_unparse(&msg->ctx));
}

static void fn_cancel(struct msg *msg)
{
    char const *ans = FAIL;
    if (!sharg_check(&msg->cmd, "s")) eparse_cleanup();
    if (!session_is_running() || session_cancel()) ans = OK;

cleanup:
    sharg_replace(&msg->ctx, "{1}", ans);
    output_put(&output, sharg_unparse(&msg->ctx));
}

static void fn_state(struct msg *msg)
{
    char const *ans = FAIL;
    char const *state = "";
    if (!sharg_check(&msg->cmd, "s")) eparse_cleanup();
    ans = OK;
    state = session_state_string();

cleanup:
    sharg_replace(&msg->ctx, "{1}", ans);
    sharg_replace(&msg->ctx, "{2}", state);
    output_put(&output, sharg_unparse(&msg->ctx));
}

static void fn_progress(struct msg *msg)
{
    char const *ans = FAIL;
    char progress[16] = {0};
    if (!sharg_check(&msg->cmd, "s")) eparse_cleanup();

    if (session_is_done())
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
        ans = FAIL;

cleanup:
    sharg_replace(&msg->ctx, "{1}", ans);
    sharg_replace(&msg->ctx, "{2}", progress);
    output_put(&output, sharg_unparse(&msg->ctx));
}

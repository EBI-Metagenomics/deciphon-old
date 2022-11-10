#include "command.h"
#include "core/command_help.h"
#include "core/fmt.h"
#include "core/logy.h"
#include "core/msg.h"
#include "core/strings.h"
#include "presser.h"
#include "pressy.h"

#define CMD_MAP(X)                                                             \
    X(CANCEL, cancel, "")                                                      \
    X(ECHO, echo, "[...]")                                                     \
    X(HELP, help, "")                                                          \
    X(PRESS, press, "HMM_FILE")                                                \
    X(PROGRESS, progress, "")                                                  \
    X(INC_PROGRESS, inc_progress, "")                                          \
    X(FILENAME, filename, "")                                                  \
    X(RESET, reset, "")                                                        \
    X(STATE, state, "")

#define COMMAND_TEMPLATE_DEF
#include "core/command_template.h"
#undef COMMAND_TEMPLATE_DEF

static void fn_cancel(struct msg *msg)
{
    if (msg_check(msg, "s")) return;

    char const *ans = presser_cancel(0) ? FAIL : OK;
    parent_send(&parent, msg_ctx(msg, ans));
}

static void fn_echo(struct msg *msg) { parent_send(&parent, msg_unparse(msg)); }

static void fn_help(struct msg *msg)
{
    unused(msg);
    command_help_init();

#define X(_, A, B) command_help_add(STRINGIFY(A), B);
    CMD_MAP(X);
#undef X

    parent_send(&parent, command_help_table());
}

static void fn_press(struct msg *msg)
{
    if (msg_check(msg, "ss")) return;

    char const *ans = FAIL;
    if (presser_is_running())
        ans = BUSY;
    else if (presser_start(msg->cmd.argv[1]))
        ans = OK;
    else
        ans = FAIL;

    parent_send(&parent, msg_ctx(msg, ans));
}

static void fn_progress(struct msg *msg)
{
    if (msg_check(msg, "s")) return;

    char const *ans = presser_is_done() || presser_is_running() ? OK : FAIL;
    char perc[] = "100%";
    char *p = fmt_percent(perc, presser_progress());
    p[0] = '%';
    p[1] = '\0';
    parent_send(&parent, msg_ctx(msg, ans, perc));
}

static void fn_inc_progress(struct msg *msg)
{
    if (msg_check(msg, "s")) return;

    char const *ans = presser_is_done() || presser_is_running() ? OK : FAIL;
    char inc[] = "100";
    fmt_percent(inc, presser_inc_progress());
    parent_send(&parent, msg_ctx(msg, ans, inc));
}

static void fn_filename(struct msg *msg)
{
    if (msg_check(msg, "s")) return;

    char const *ans = presser_is_done() || presser_is_running() ? OK : FAIL;
    char const *filename = presser_filename();
    parent_send(&parent, msg_ctx(msg, ans, filename));
}

static void fn_reset(struct msg *msg)
{
    if (msg_check(msg, "s")) return;

    char const *ans = presser_is_running() ? FAIL : OK;
    presser_reset();
    parent_send(&parent, msg_ctx(msg, ans));
}

static void fn_state(struct msg *msg)
{
    if (msg_check(msg, "s")) return;
    char const *ans = OK;
    char const *state = presser_state_string();
    parent_send(&parent, msg_ctx(msg, ans, state, presser_filename()));
}

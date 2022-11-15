#include "command.h"
#include "core/command_help.h"
#include "core/global.h"
#include "core/msg.h"
#include "core/strings.h"
#include "hmmer.h"
#include "hmmy.h"

#define CMD_MAP(X)                                                             \
    X(ECHO, echo, "[...]")                                                     \
    X(HELP, help, "")                                                          \
    X(START, start, "HMM_FILE")                                                \
    X(STOP, stop, "")                                                          \
    X(STATE, state, "")                                                        \
    X(SCAN, scan, "")                                                          \
    X(QUIT, quit, "")

#define COMMAND_TEMPLATE_DEF
#include "core/command_template.h"
#undef COMMAND_TEMPLATE_DEF

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

static void fn_start(struct msg *msg)
{
    if (msg_check(msg, "ss")) return;
    hmmer_start(msg_str(msg, 1));
}

static void fn_stop(struct msg *msg)
{
    if (msg_check(msg, "s")) return;
    hmmer_stop();
}

static void fn_state(struct msg *msg)
{
    if (msg_check(msg, "s")) return;
    if (hmmer_state() == HMMER_OFF) parent_send(&parent, "off");
    if (hmmer_state() == HMMER_BOOT) parent_send(&parent, "boot");
    if (hmmer_state() == HMMER_READY) parent_send(&parent, "ready");
    if (hmmer_state() == HMMER_RUN) parent_send(&parent, "run");
    if (hmmer_state() == HMMER_DONE) parent_send(&parent, "done");
    if (hmmer_state() == HMMER_FAIL) parent_send(&parent, "fail");
}

static void fn_scan(struct msg *msg)
{
    if (msg_check(msg, "ss")) return;
}

static void fn_quit(struct msg *msg)
{
    if (msg_check(msg, "s")) return;
    hmmer_stop();
    global_terminate();
}

#include "command.h"
#include "core/as.h"
#include "core/command_help.h"
#include "core/fmt.h"
#include "core/logy.h"
#include "core/msg.h"
#include "core/strings.h"
#include "scanner.h"
#include "scanny.h"

#define CMD_MAP(X)                                                             \
    X(HELP, help, "")                                                          \
    X(ECHO, echo, "[...]")                                                     \
    X(SET_NTHREADS, set_nthreads, "NTHREADS")                                  \
    X(SCAN, scan, "SEQS_FILE DB_FILE PROD_FILE MULTI_HITS HMMER3_COMPAT")      \
    X(CANCEL, cancel, "")                                                      \
    X(STATE, state, "")                                                        \
    X(PROGRESS, progress, "")                                                  \
    X(INC_PROGRESS, inc_progress, "")                                          \
    X(FILENAME, filename, "")                                                  \
    X(RESET, reset, "")

#define COMMAND_TEMPLATE_DEF
#include "core/command_template.h"
#undef COMMAND_TEMPLATE_DEF

static void fn_echo(struct msg *msg) { parent_send(&parent, msg_unparse(msg)); }

static void fn_help(struct msg *msg)
{
    UNUSED(msg);
    command_help_init();

#define X(_, A, B) command_help_add(STRINGIFY(A), B);
    CMD_MAP(X);
#undef X

    parent_send(&parent, command_help_table());
}

static void fn_set_nthreads(struct msg *msg)
{
    if (msg_check(msg, "si")) return;

    scanner_set_nthreads(as_int(msg->cmd.argv[1]));
    parent_send(&parent, msg_ctx(msg, "ok"));
}

static void fn_scan(struct msg *msg)
{
    if (msg_check(msg, "ssssii")) return;

    char const *ans = FAIL;
    if (scanner_is_running())
    {
        ans = BUSY;
        goto cleanup;
    }

    char const *seqs = msg->cmd.argv[1];
    char const *db = msg->cmd.argv[2];
    char const *prod = msg->cmd.argv[3];
    bool multi_hits = !!as_long(msg->cmd.argv[4]);
    bool hmmer3_compat = !!as_long(msg->cmd.argv[5]);
    if (scanner_start(seqs, db, prod, multi_hits, hmmer3_compat)) ans = OK;

cleanup:
    parent_send(&parent, msg_ctx(msg, ans));
}

static void fn_cancel(struct msg *msg)
{
    if (msg_check(msg, "s")) return;

    char const *ans = scanner_cancel(0) ? FAIL : OK;
    parent_send(&parent, msg_ctx(msg, ans));
}

static void fn_state(struct msg *msg)
{
    if (msg_check(msg, "s")) return;

    char const *state = scanner_state_string();
    parent_send(&parent, msg_ctx(msg, OK, state));
}

static void fn_progress(struct msg *msg)
{
    if (msg_check(msg, "s")) return;

    char const *ans = scanner_is_done() || scanner_is_running() ? OK : FAIL;
    char perc[] = "100%";
    fmt_percent(perc, scanner_progress());
    parent_send(&parent, msg_ctx(msg, ans, perc));
}

static void fn_inc_progress(struct msg *msg)
{
    if (msg_check(msg, "s")) return;

    char const *ans = scanner_is_done() || scanner_is_running() ? OK : FAIL;
    char inc[] = "100";
    fmt_percent(inc, scanner_inc_progress());
    parent_send(&parent, msg_ctx(msg, ans, inc));
}

static void fn_filename(struct msg *msg)
{
    if (msg_check(msg, "s")) return;

    char const *ans = scanner_is_done() || scanner_is_running() ? OK : FAIL;
    char const *filename = scanner_filename();
    parent_send(&parent, msg_ctx(msg, ans, filename));
}

static void fn_reset(struct msg *msg)
{
    if (msg_check(msg, "s")) return;

    char const *ans = scanner_is_running() ? FAIL : OK;
    scanner_reset();
    parent_send(&parent, msg_ctx(msg, ans));
}

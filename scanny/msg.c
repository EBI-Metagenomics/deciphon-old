#include "msg.h"
#include "core/as.h"
#include "core/logy.h"
#include "core/service_strings.h"
#include "core/strings.h"
#include "scanny.h"
#include "session.h"
#include "zc.h"
#include <stdio.h>

#define MSG_MAP(X)                                                             \
    X(INVALID, invalid, "")                                                    \
    X(HELP, help, "")                                                          \
    X(ECHO, echo, "[...]")                                                     \
    X(SET_NTHREADS, set_nthreads, "NTHREADS")                                  \
    X(SCAN, scan, "SEQS_FILE DB_FILE PROD_FILE MULTI_HITS HMMER3_COMPAT")      \
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

static void fn_set_nthreads(struct msg *msg)
{
    char const *ans = FAIL;
    if (!sharg_check(&msg->cmd, "si")) eparse_cleanup();
    session_set_nthreads(as_int64(msg->cmd.argv[1]));
    ans = OK;

cleanup:
    sharg_replace(&msg->ctx, "{1}", ans);
    output_put(&output, sharg_unparse(&msg->ctx));
}

static void fn_scan(struct msg *msg)
{
    char const *ans = FAIL;
    if (!sharg_check(&msg->cmd, "ssssii")) eparse_cleanup();

    if (session_is_running())
    {
        ans = BUSY;
        goto cleanup;
    }

    char const *seqs = msg->cmd.argv[1];
    char const *db = msg->cmd.argv[2];
    char const *prod = msg->cmd.argv[3];
    bool multi_hits = !!as_int64(msg->cmd.argv[4]);
    bool hmmer3_compat = !!as_int64(msg->cmd.argv[5]);
    if (session_start(seqs, db, prod, multi_hits, hmmer3_compat)) ans = OK;

cleanup:
    sharg_replace(&msg->ctx, "{1}", ans);
    output_put(&output, sharg_unparse(&msg->ctx));
}

static void fn_cancel(struct msg *msg)
{
    char const *ans = FAIL;
    if (!sharg_check(&msg->cmd, "s")) eparse_cleanup();

    if (!session_is_running())
    {
        ans = OK;
        goto cleanup;
    }
    if (session_cancel()) ans = OK;

cleanup:
    sharg_replace(&msg->ctx, "{1}", ans);
    output_put(&output, sharg_unparse(&msg->ctx));
}

static void fn_state(struct msg *msg)
{
    char const *ans = FAIL;
    char const *state = "";
    if (!sharg_check(&msg->cmd, "s")) eparse_cleanup();

    state = session_state_string();
    ans = OK;

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

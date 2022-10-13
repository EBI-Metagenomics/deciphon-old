#include "msg.h"
#include "core/as.h"
#include "core/logy.h"
#include "session.h"
#include "strings.h"
#include <stdio.h>

#define MSG_MAP(X)                                                             \
    X(INVALID, invalid, "")                                                    \
    X(HELP, help, "")                                                          \
    X(SET_NTHREADS, set_nthreads, "NTHREADS")                                  \
    X(SCAN, scan, "SEQS_FILE DB_FILE PROD_FILE MULTI_HITS HMMER3_COMPAT")      \
    X(CANCEL, cancel, "")                                                      \
    X(STATE, state, "")                                                        \
    X(PROGRESS, progress, "")

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

static char const *fn_set_nthreads(struct msg *msg)
{
    if (!sharg_check(&msg->cmd, "si")) return eparse(FAIL_PARSE), FAIL;
    session_set_nthreads(as_int64(msg->cmd.argv[1]));
    return OK;
}

static char const *fn_scan(struct msg *msg)
{
    if (!sharg_check(&msg->cmd, "ssssii")) return eparse(FAIL_PARSE), FAIL;

    if (session_is_running()) return BUSY;
    char const *seqs = msg->cmd.argv[1];
    char const *db = msg->cmd.argv[2];
    char const *prod = msg->cmd.argv[3];
    bool multi_hits = !!as_int64(msg->cmd.argv[4]);
    bool hmmer3_compat = !!as_int64(msg->cmd.argv[5]);
    return session_start(seqs, db, prod, multi_hits, hmmer3_compat) ? OK : FAIL;
}

static char const *fn_cancel(struct msg *msg)
{
    if (!sharg_check(&msg->cmd, "s")) return eparse(FAIL_PARSE), FAIL;
    if (!session_is_running()) return DONE;
    return session_cancel() ? OK : FAIL;
}

static char const *fn_state(struct msg *msg)
{
    if (!sharg_check(&msg->cmd, "s")) return eparse(FAIL_PARSE), FAIL;
    return session_state_string();
}

static char const *fn_progress(struct msg *msg)
{
    if (!sharg_check(&msg->cmd, "s")) return eparse(FAIL_PARSE), FAIL;

    static char progress[5] = "100%";
    if (session_is_done()) return "100%";

    if (session_is_running())
    {
        int perc = session_progress();
        if (perc < 0) return FAIL;
        sprintf(progress, "%u%%", (unsigned)perc);
        return progress;
    }

    return FAIL;
}

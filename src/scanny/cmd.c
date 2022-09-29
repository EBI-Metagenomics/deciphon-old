#include "scanny/cmd.h"
#include "core/logging.h"
#include "scanny/session.h"
#include "scanny/strings.h"

#define CMD_MAP(X)                                                             \
    X(INVALID, invalid, "")                                                    \
    X(HELP, help, "")                                                          \
    X(SET_NTHREADS, set_nthreads, "NTHREADS")                                  \
    X(SCAN, scan, "SEQS_FILE DB_FILE PROD_FILE MULTI_HITS HMMER3_COMPAT")      \
    X(CANCEL, cancel, "")                                                      \
    X(STATE, state, "")                                                        \
    X(PROGRESS, progress, "")

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

static char const *fn_set_nthreads(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "si")) return eparse(FAIL_PARSE), FAIL;
    session_set_nthreads(cmd_as_i64(cmd, 1));
    return OK;
}

static char const *fn_scan(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "ssssii")) return eparse(FAIL_PARSE), FAIL;

    if (session_is_running()) return BUSY;
    char const *seqs = cmd_get(cmd, 1);
    char const *db = cmd_get(cmd, 2);
    char const *prod = cmd_get(cmd, 3);
    bool multi_hits = !!cmd_as_i64(cmd, 4);
    bool hmmer3_compat = !!cmd_as_i64(cmd, 5);
    return session_start(seqs, db, prod, multi_hits, hmmer3_compat) ? OK : FAIL;
}

static char const *fn_cancel(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "s")) return eparse(FAIL_PARSE), FAIL;
    if (!session_is_running()) return DONE;
    return session_cancel() ? OK : FAIL;
}

static char const *fn_state(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "s")) return eparse(FAIL_PARSE), FAIL;
    return session_state_string();
}

static char const *fn_progress(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "s")) return eparse(FAIL_PARSE), FAIL;

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

#include "scanny/cmd.h"
#include "core/c23.h"
#include "core/logging.h"
#include "db/profile_reader.h"
#include "db/protein_reader.h"
#include "jx.h"
#include "scanny/session.h"
#include "uv.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

static cmd_fn_t *scanny_cmds[] = {
#define X(_1, A, _2) &scanny_cmd_##A,
    SCANNY_CMD_MAP(X)
#undef X
};

static enum scanny_cmd parse(char const *cmd)
{
#define X(A, B, _)                                                             \
    if (!strcmp(cmd, STRINGIFY(B))) return SCANNY_CMD_##A;
    SCANNY_CMD_MAP(X)
#undef X
    return SCANNY_CMD_INVALID;
}

static inline char const *say_ok(void) { return "OK"; }

static inline char const *say_done(void) { return "DONE"; }

static inline char const *say_fail(void) { return "FAIL"; }

static inline char const *say_busy(void) { return "BUSY"; }

#define error_parse() error("failed to parse command")

cmd_fn_t *scanny_cmd(char const *cmd) { return scanny_cmds[parse(cmd)]; }

char const *scanny_cmd_invalid(struct cmd const *cmd)
{
    (void)cmd;
    eparse("invalid command");
    return say_fail();
}

static char help_table[512] = {0};

char const *scanny_cmd_help(struct cmd const *cmd)
{
    (void)cmd;
    char *p = help_table;
    p += sprintf(p, "Commands:");

#define X(_, A, B)                                                             \
    if (strcmp(STRINGIFY(A), "invalid"))                                       \
        p += sprintf(p, "\n  %-11s %s", STRINGIFY(A), B);
    SCANNY_CMD_MAP(X);
#undef X

    return help_table;
}

char const *scanny_cmd_set_nthreads(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "si"))
    {
        error_parse();
        return say_fail();
    }
    scanny_session_set_nthreads(cmd_get_i64(cmd, 1));
    return say_ok();
}

char const *scanny_cmd_scan(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "ssssii"))
    {
        error_parse();
        return say_fail();
    }

    if (scanny_session_is_running()) return say_busy();
    char const *seqs = cmd->argv[1];
    char const *db = cmd->argv[2];
    char const *prod = cmd->argv[3];
    bool multi_hits = !!cmd_get_i64(cmd, 4);
    bool hmmer3_compat = !!cmd_get_i64(cmd, 5);
    return scanny_session_start(seqs, db, prod, multi_hits, hmmer3_compat)
               ? say_ok()
               : say_fail();
}

char const *scanny_cmd_cancel(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "s"))
    {
        error_parse();
        return say_fail();
    }

    if (!scanny_session_is_running()) return say_done();

    return scanny_session_cancel() ? say_ok() : say_fail();
}

char const *scanny_cmd_state(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "s"))
    {
        error_parse();
        return say_fail();
    }

    return scanny_session_state_string();
}

char const *scanny_cmd_progress(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "s"))
    {
        error_parse();
        return say_fail();
    }

    static char progress[5] = "100%";
    if (scanny_session_is_done()) return "100%";

    if (scanny_session_is_running())
    {
        int perc = scanny_session_progress();
        if (perc < 0) return say_fail();
        sprintf(progress, "%u%%", (unsigned)perc);
        return progress;
    }

    return say_fail();
}

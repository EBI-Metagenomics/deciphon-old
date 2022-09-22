#include "scanny_cmd.h"
#include "core/c23.h"
#include "core/logging.h"
#include "db/profile_reader.h"
#include "db/protein_reader.h"
#include "jx.h"
#include "scanny_session.h"
#include "uv.h"
#include "xfile.h"
#include "zc.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

static cmd_fn_t *scanny_cmds[] = {
#define X(_, A) &A,
    SCANNY_CMD_MAP(X)
#undef X
};

static inline char const *say_ok(void) { return "OK"; }

static inline char const *say_done(void) { return "DONE"; }

static inline char const *say_fail(void) { return "FAIL"; }

static inline char const *say_busy(void) { return "BUSY"; }

#define error_parse() error("failed to parse command")

static enum scanny_cmd parse(char const *);

cmd_fn_t *scanny_cmd(char const *cmd) { return scanny_cmds[parse(cmd)]; }

char const *scanny_cmd_invalid(struct cmd const *cmd)
{
    (void)cmd;
    eparse("invalid command");
    return say_fail();
}

char const *scanny_cmd_scan(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "sssii"))
    {
        error_parse();
        return say_fail();
    }

    if (scanny_session_is_running()) return say_busy();
    char const *seqs = cmd->argv[1];
    char const *db = cmd->argv[2];
    bool multi_hits = !!cmd_get_i64(cmd, 3);
    bool hmmer3_compat = !!cmd_get_i64(cmd, 4);
    return scanny_session_start(seqs, db, multi_hits, hmmer3_compat)
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
    if (scanny_session_is_done()) return progress;

    if (scanny_session_is_running())
    {
        sprintf(progress, "%u%%", scanny_session_progress());
        return progress;
    }

    return say_fail();
}

static enum scanny_cmd parse(char const *cmd)
{
#define X(A, _)                                                                \
    if (!strcmp(cmd, STRINGIFY(A))) return SCANNY_CMD_##A;
    SCANNY_CMD_MAP(X)
#undef X
    return SCANNY_CMD_INVALID;
}

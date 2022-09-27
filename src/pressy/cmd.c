#include "pressy/cmd.h"
#include "core/c23.h"
#include "core/logging.h"
#include "db/press.h"
#include "pressy/pressy.h"
#include "pressy/session.h"
#include "uv.h"
#include "xfile.h"
#include <stdatomic.h>
#include <string.h>

static cmd_fn_t *pressy_cmds[] = {
#define X(_1, A, _2) &pressy_cmd_##A,
    PRESSY_CMD_MAP(X)
#undef X
};

static enum pressy_cmd parse(char const *cmd)
{
#define X(A, B, _)                                                             \
    if (!strcmp(cmd, STRINGIFY(B))) return PRESSY_CMD_##A;
    PRESSY_CMD_MAP(X)
#undef X
    return PRESSY_CMD_INVALID;
}

static inline char const *say_ok(void) { return "OK"; }

static inline char const *say_done(void) { return "DONE"; }

static inline char const *say_fail(void) { return "FAIL"; }

static inline char const *say_busy(void) { return "BUSY"; }

#define error_parse() error("failed to parse command")

cmd_fn_t *pressy_cmd(char const *cmd) { return pressy_cmds[parse(cmd)]; }

char const *pressy_cmd_invalid(struct cmd const *cmd)
{
    (void)cmd;
    eparse("invalid command");
    return say_fail();
}

static char help_table[512] = {0};

char const *pressy_cmd_help(struct cmd const *cmd)
{
    (void)cmd;
    char *p = help_table;
    p += sprintf(p, "Commands:");

#define X(_, A, B)                                                             \
    if (strcmp(STRINGIFY(A), "invalid"))                                       \
        p += sprintf(p, "\n  %-11s %s", STRINGIFY(A), B);
    PRESSY_CMD_MAP(X);
#undef X

    return help_table;
}

char const *pressy_cmd_press(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "ss"))
    {
        error_parse();
        return say_fail();
    }

    if (pressy_session_is_running()) return say_busy();
    return pressy_session_start(cmd->argv[1]) ? say_ok() : say_fail();
}

char const *pressy_cmd_cancel(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "s"))
    {
        error_parse();
        return say_fail();
    }

    if (!pressy_session_is_running()) return say_done();

    return pressy_session_cancel() ? say_ok() : say_fail();
}

char const *pressy_cmd_state(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "s"))
    {
        error_parse();
        return say_fail();
    }

    return pressy_session_state_string();
}

char const *pressy_cmd_progress(struct cmd const *cmd)
{
    if (!cmd_check(cmd, "s"))
    {
        error_parse();
        return say_fail();
    }

    static char progress[5] = "100%";
    if (pressy_session_is_done()) return progress;

    if (pressy_session_is_running())
    {
        sprintf(progress, "%u%%", pressy_session_progress());
        return progress;
    }

    return say_fail();
}

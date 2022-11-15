#include "command.h"
#include "core/cmd.h"
#include "core/fmt.h"
#include "core/logy.h"
#include "core/msg.h"
#include "core/pp.h"
#include "core/strings.h"
#include "presser.h"
#include "pressy.h"

#define CMD_MAP(X)                                                             \
    X(cancel, "")                                                              \
    X(echo, "[...]")                                                           \
    X(help, "")                                                                \
    X(press, "HMM_FILE")                                                       \
    X(progress, "")                                                            \
    X(inc_progress, "")                                                        \
    X(filename, "")                                                            \
    X(reset, "")                                                               \
    X(state, "")

#define X(A, _) static void A(struct msg *);
CMD_MAP(X)
#undef X

static struct cmd_entry entries[] = {
#define X(A, B) {&A, stringify(A), B},
    CMD_MAP(X)
#undef X
};

void command_call(struct msg *msg)
{
    struct cmd_entry *e = cmd_find(array_size(entries), entries, msg_cmd(msg));
    if (!e)
    {
        einval("unrecognized command: %s", msg_cmd(msg));
        return;
    }

    msg_shift(msg);
    (*e->func)(msg);
}

static void cancel(struct msg *msg)
{
    char const *ans = presser_cancel(0) ? FAIL : OK;
    parent_send(&parent, msg_ctx(msg, ans));
}

static void echo(struct msg *msg) { parent_send(&parent, msg_unparse(msg)); }

static void help(struct msg *msg)
{
    unused(msg);
    cmd_help_init();

    for (size_t i = 0; i < array_size(entries); ++i)
        cmd_help_add(entries[i].name, entries[i].doc);

    parent_send(&parent, cmd_help_table());
}

static void press(struct msg *msg)
{
    if (msg_check(msg, "s")) return;

    char const *ans = FAIL;
    if (presser_is_running())
        ans = BUSY;
    else if (presser_start(msg_str(msg, 0)))
        ans = OK;
    else
        ans = FAIL;

    parent_send(&parent, msg_ctx(msg, ans));
}

static void progress(struct msg *msg)
{
    char const *ans = presser_is_done() || presser_is_running() ? OK : FAIL;
    char perc[] = "100%";
    char *p = fmt_perc(perc, presser_progress());
    p[0] = '%';
    p[1] = '\0';
    parent_send(&parent, msg_ctx(msg, ans, perc));
}

static void inc_progress(struct msg *msg)
{
    char const *ans = presser_is_done() || presser_is_running() ? OK : FAIL;
    char inc[] = "100";
    fmt_perc(inc, presser_inc_progress());
    parent_send(&parent, msg_ctx(msg, ans, inc));
}

static void filename(struct msg *msg)
{
    char const *ans = presser_is_done() || presser_is_running() ? OK : FAIL;
    char const *name = presser_filename();
    parent_send(&parent, msg_ctx(msg, ans, name));
}

static void reset(struct msg *msg)
{
    char const *ans = presser_is_running() ? FAIL : OK;
    presser_reset();
    parent_send(&parent, msg_ctx(msg, ans));
}

static void state(struct msg *msg)
{
    char const *ans = OK;
    char const *state = presser_state_string();
    parent_send(&parent, msg_ctx(msg, ans, state, presser_filename()));
}

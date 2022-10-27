#include "command.h"
#include "broker.h"
#include "core/command_help.h"
#include "core/file.h"
#include "core/fmt.h"
#include "core/logy.h"
#include "core/msg.h"
#include "core/sched.h"
#include "core/sched_dump.h"
#include "core/strings.h"
#include "decy.h"
#include "fs.h"
#include <string.h>

#define CMD_MAP(X)                                                             \
    X(HELP, help, "")                                                          \
    X(ECHO, echo, "[...]")                                                     \
    X(FWD, fwd, "DST [...]")

// X(PRESSY, pressy, "PRESSY_COMMAND [...]")                                  \
    // X(SCANNY, scanny, "SCANNY_COMMAND [...]")                                  \
    // X(SCHEDY, schedy, "SCHEDY_COMMAND [...]")                                  \
    // X(EXEC_PEND_JOB, exec_pend_job, "ANS JOB_JSON")                            \
    // X(EXEC_PRESS_JOB, exec_press_job, "ANS JOB_JSON")                          \
    // X(EXEC_SCAN_JOB, exec_scan_job, "ANS JOB_JSON")

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

static void fn_fwd(struct msg *msg)
{
    if (msg_check(msg, "sss*")) return;
    msg_shift(msg);

    int id = -1;
    if (!strcmp(msg_argv(msg)[0], "parent"))

        broker_send(PRESSY_ID, msg_unparse(msg));
}

#if 0
static void fn_scanny(struct msg *msg)
{
    if (msg_check(msg, "ss*")) return;
    msg_shift(msg);
    broker_send(SCANNY_ID, msg_unparse(msg));
}

static void fn_schedy(struct msg *msg)
{
    if (msg_check(msg, "ss*")) return;
    msg_shift(msg);
    broker_send(SCHEDY_ID, msg_unparse(msg));
}

static void fn_exec_pend_job(struct msg *msg)
{
    if (msg_check(msg, "sss")) return;
    if (strcmp(msg->cmd.argv[1], "ok"))
    {
        error("expected ok but got %s", msg->cmd.argv[1]);
        goto cleanup;
    }

    char *json = msg->cmd.argv[2];
    if (!broker_parse_job(&job, json)) goto cleanup;

    if (sched_job_type(&job) != SCHED_SCAN && sched_job_type(&job) != SCHED_HMM)
    {
        error("unrecognized job type");
        goto cleanup;
    }

    if (sched_job_type(&job) == SCHED_SCAN)
        broker_send(
            SCHEDY_ID,
            fmt("job_set_state %lld run | exec_scan_job {1} {2}", job.id));

    if (sched_job_type(&job) == SCHED_HMM)
        broker_send(
            SCHEDY_ID,
            fmt("job_set_state %lld run | exec_press_job {1} {2}", job.id));

cleanup:
    return;
}

static void fn_exec_press_job(struct msg *msg)
{
    if (msg_check(msg, "sss")) return;
    if (strcmp(msg->cmd.argv[1], "ok"))
    {
        error("expected ok but got %s", msg->cmd.argv[1]);
        goto cleanup;
    }

    char *json = msg->cmd.argv[2];
    if (!broker_parse_job(&job, json)) goto cleanup;

    if (sched_job_type(&job) != SCHED_HMM)
    {
        error("unexpected job type");
        goto cleanup;
    }

cleanup:
    return;
}

static void fn_exec_scan_job(struct msg *msg)
{
    if (msg_check(msg, "sss")) return;
    if (strcmp(msg->cmd.argv[1], "ok"))
    {
        error("expected ok but got %s", msg->cmd.argv[1]);
        goto cleanup;
    }

    char *json = msg->cmd.argv[2];
    if (!broker_parse_job(&job, json)) goto cleanup;

    if (sched_job_type(&job) != SCHED_SCAN)
    {
        error("unexpected job type");
        goto cleanup;
    }

cleanup:
    return;
}
#endif

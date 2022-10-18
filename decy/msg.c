#include "msg.h"
#include "broker.h"
#include "core/file.h"
#include "core/fmt.h"
#include "core/logy.h"
#include "core/sched.h"
#include "core/sched_dump.h"
#include "core/service_strings.h"
#include "core/strings.h"
#include "decy.h"
#include "xfile.h"
#include <string.h>

#define MSG_MAP(X)                                                             \
    X(INVALID, invalid, "")                                                    \
    X(HELP, help, "")                                                          \
    X(ECHO, echo, "[...]")                                                     \
    X(PRESSY, pressy, "PRESSY_COMMAND [...]")                                  \
    X(SCANNY, scanny, "SCANNY_COMMAND [...]")                                  \
    X(SCHEDY, schedy, "SCHEDY_COMMAND [...]")                                  \
    X(EXEC_PEND_JOB, exec_pend_job, "ANS JOB_JSON")                            \
    X(EXEC_PRESS_JOB, exec_press_job, "ANS JOB_JSON")                          \
    X(EXEC_SCAN_JOB, exec_scan_job, "ANS JOB_JSON")

#define MSG_TEMPLATE_ENABLE
#include "core/msg_template.h"
#undef MSG_TEMPLATE_ENABLE

static struct sched_job job = {0};

static void fn_invalid(struct msg *msg)
{
    warn("invalid command: %s", msg->cmd.argv[0]);
    sharg_replace(&msg->ctx, "{1}", OK);
    output_put(&output, sharg_unparse(&msg->ctx));
}

static void fn_help(struct msg *msg)
{
    UNUSED(msg);
    static char help_table[1024] = {0};
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

static void fn_pressy(struct msg *msg)
{
    debug("forwarding to pressy");
    if (!sharg_check(&msg->cmd, "ss*")) eparse_cleanup();
    sharg_shift(&msg->cmd);
    broker_send(PRESSY_ID, msg_unparse(msg));
cleanup:
    return;
}

static void fn_scanny(struct msg *msg)
{
    debug("forwarding to scanny");
    if (!sharg_check(&msg->cmd, "ss*")) eparse_cleanup();
    sharg_shift(&msg->cmd);
    broker_send(SCANNY_ID, msg_unparse(msg));
cleanup:
    return;
}

static void fn_schedy(struct msg *msg)
{
    debug("forwarding to schedy");
    if (!sharg_check(&msg->cmd, "ss*")) eparse_cleanup();
    sharg_shift(&msg->cmd);
    broker_send(SCHEDY_ID, msg_unparse(msg));
cleanup:
    return;
}

static void fn_exec_pend_job(struct msg *msg)
{
    if (!sharg_check(&msg->cmd, "sss")) eparse_cleanup();
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
    if (!sharg_check(&msg->cmd, "sss")) eparse_cleanup();
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
    if (!sharg_check(&msg->cmd, "sss")) eparse_cleanup();
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

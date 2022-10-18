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

static char const *fn_invalid(struct msg *msg)
{
    warn("invalid command: %s", msg->cmd.argv[0]);
    return msg_unparse(msg);
}

static char const *fn_help(struct msg *msg)
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

    return help_table;
}

#define eparse_cleanup()                                                       \
    do                                                                         \
    {                                                                          \
        eparse(INVALID_ARGS);                                                  \
        goto cleanup;                                                          \
    } while (0);

static char const *fn_pressy(struct msg *msg)
{
    char const *ans = FAIL;
    if (!sharg_check(&msg->cmd, "ss*")) eparse_cleanup();
    ans = broker_forward_msg(sharg_shift(&msg->cmd), msg);

cleanup:
    sharg_replace(&msg->ctx, "{1}", ans);
    return sharg_unparse(&msg->ctx);
}

static char const *fn_scanny(struct msg *msg)
{
    char const *ans = FAIL;
    if (!sharg_check(&msg->cmd, "ss*")) eparse_cleanup();
    ans = broker_forward_msg(sharg_shift(&msg->cmd), msg);

cleanup:
    sharg_replace(&msg->ctx, "{1}", ans);
    return sharg_unparse(&msg->ctx);
}

static char const *fn_schedy(struct msg *msg)
{
    char const *ans = FAIL;
    if (!sharg_check(&msg->cmd, "ss*")) eparse_cleanup();
    ans = broker_forward_msg(sharg_shift(&msg->cmd), msg);

cleanup:
    sharg_replace(&msg->ctx, "{1}", ans);
    return sharg_unparse(&msg->ctx);
}

static char const *fn_exec_pend_job(struct msg *msg)
{
    char const *ans = FAIL;
    if (!sharg_check(&msg->cmd, "sss")) eparse_cleanup();
    if (strcmp(msg->cmd.argv[1], "ok")) return NULL;

    char *json = msg->cmd.argv[2];
    if (!broker_parse_job(&job, json)) goto cleanup;

    if (sched_job_type(&job) != SCHED_SCAN && sched_job_type(&job) != SCHED_HMM)
    {
        error("unrecognized job type");
        goto cleanup;
    }

    ans = OK;

    if (sched_job_type(&job) == SCHED_SCAN)
        broker_send(
            SCHEDY_ID,
            fmt("job_set_state %lld run | exec_scan_job {1} {2}", job.id));

    if (sched_job_type(&job) == SCHED_HMM)
        broker_send(
            SCHEDY_ID,
            fmt("job_set_state %lld run | exec_press_job {1} {2}", job.id));

cleanup:
    sharg_replace(&msg->ctx, "{1}", ans);
    return sharg_unparse(&msg->ctx);
}

static char const *fn_exec_press_job(struct msg *msg)
{
    char const *ans = FAIL;
    if (!sharg_check(&msg->cmd, "sss")) eparse_cleanup();
    if (strcmp(msg->cmd.argv[1], "ok")) return NULL;

    char *json = msg->cmd.argv[2];
    if (!broker_parse_job(&job, json)) goto cleanup;

    if (sched_job_type(&job) != SCHED_HMM)
    {
        error("unexpected job type");
        goto cleanup;
    }

    ans = OK;

cleanup:
    sharg_replace(&msg->ctx, "{1}", ans);
    return sharg_unparse(&msg->ctx);
}

static char const *fn_exec_scan_job(struct msg *msg)
{
    char const *ans = FAIL;
    if (!sharg_check(&msg->cmd, "sss")) eparse_cleanup();
    if (strcmp(msg->cmd.argv[1], "ok")) return NULL;

    char *json = msg->cmd.argv[2];
    if (!broker_parse_job(&job, json)) goto cleanup;

    if (sched_job_type(&job) != SCHED_SCAN)
    {
        error("unexpected job type");
        goto cleanup;
    }

    ans = OK;

cleanup:
    sharg_replace(&msg->ctx, "{1}", ans);
    return sharg_unparse(&msg->ctx);
}

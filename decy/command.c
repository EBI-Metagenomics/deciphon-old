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
#include "fs.h"
#include <string.h>

#define CMD_MAP(X)                                                             \
    X(HELP, help, "")                                                          \
    X(ECHO, echo, "[...]")                                                     \
    X(FWD, fwd, "DST [...]")                                                   \
    X(PRESSY_POLLING, pressy_polling, "TODO")                                  \
    X(PRESSY_AVAIL_ANSWER, pressy_avail_answer, "TODO")                        \
    X(SCHEDY_POLLING, schedy_polling, "TODO")                                  \
    X(UPDATE_PRESS_PROGRESS_1, update_press_progress_1, "TODO")                \
    X(UPDATE_PRESS_PROGRESS_2, update_press_progress_2, "TODO")                \
    X(UPDATE_PRESS_PROGRESS_3, update_press_progress_3, "TODO")                \
    X(UPDATE_PRESS_STATE_1, update_press_state_1, "TODO")                      \
    X(UPDATE_PRESS_STATE_2, update_press_state_2, "TODO")                      \
    X(UPDATE_PRESS_STATE_3, update_press_state_3, "TODO")                      \
    X(EXEC_PRESS_JOB_1, exec_press_job_1, "TODO")                              \
    X(EXEC_PRESS_JOB_2, exec_press_job_2, "TODO")                              \
    X(EXEC_PRESS_JOB_3, exec_press_job_3, "TODO")

// X(PRESSY, pressy, "PRESSY_COMMAND [...]")                                  \
    // X(SCANNY, scanny, "SCANNY_COMMAND [...]")                                  \
    // X(SCHEDY, schedy, "SCHEDY_COMMAND [...]")                                  \
    // X(EXEC_SCAN_JOB, exec_scan_job, "ANS JOB_JSON")

#define COMMAND_TEMPLATE_DEF
#include "core/command_template.h"
#undef COMMAND_TEMPLATE_DEF

static struct sched_job job = {0};
static struct sched_hmm hmm = {0};
static char buffer[1024] = {0};

static void fn_echo(struct msg *msg)
{
    broker_send(PARENT_ID, msg_unparse(msg));
}

static void fn_help(struct msg *msg)
{
    UNUSED(msg);
    command_help_init();

#define X(_, A, B) command_help_add(STRINGIFY(A), B);
    CMD_MAP(X);
#undef X

    broker_send(PARENT_ID, command_help_table());
}

static void fn_fwd(struct msg *msg)
{
    if (msg_check(msg, "sss*")) return;
    msg_shift(msg);

    int id = broker_resolve_procname(msg_argv(msg)[0]);
    if (id < 0) return;

    msg_shift(msg);
    broker_send(id, msg_unparse(msg));
}

static void fn_schedy_polling(struct msg *msg)
{
    if (msg_check(msg, "sss")) return;
    if (!strcmp(msg_argv(msg)[1], "end")) return;
    if (strcmp(msg_argv(msg)[1], "ok"))
    {
        error("unexpected response: <%s>", msg_unparse(msg));
        return;
    }

    char *json = msg_argv(msg)[2];
    if (!broker_parse_job(&job, json)) return;

    // if (sched_job_type(&job) == SCHED_SCAN)
    //     broker_send(
    //         SCHEDY_ID,
    //         fmt("job_set_state %lld run | exec_scan_job {1} {2}", job.id));

    if (sched_job_type(&job) == SCHED_HMM)
        broker_send(PRESSY_ID,
                    fmt("state | pressy_avail_answer {1} {2} %lld", job.id));
}

static void fn_pressy_polling(struct msg *msg)
{
    if (msg_check(msg, "sss")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;
    if (!strcmp(msg_argv(msg)[2], "idle")) return;

    if (!strcmp(msg_argv(msg)[2], "run"))
    {
        broker_send(PRESSY_ID,
                    fmt("inc_progress | update_press_progress_1 {1} {2}"));
    }
    else
    {
        char const *state = msg_argv(msg)[2];
        broker_send(PRESSY_ID,
                    fmt("filename | update_press_state_1 {1} {2} %s", state));
    }
}

static void fn_update_press_state_1(struct msg *msg)
{
    if (msg_check(msg, "ssss")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;
    char const *filename = msg_argv(msg)[2];
    char const *state = msg_argv(msg)[3];
    broker_send(PRESSY_ID,
                fmt("reset | update_press_state_2 {1} %s %s", filename, state));
}

static void fn_update_press_state_2(struct msg *msg)
{
    if (msg_check(msg, "ssss")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;
    char const *filename = msg_argv(msg)[2];
    char const *state = msg_argv(msg)[3];
    broker_send(SCHEDY_ID,
                fmt("hmm_get_by_filename %s | update_press_state_3 {1} {2} %s",
                    filename, state));
}

static void fn_update_press_state_3(struct msg *msg)
{
    if (msg_check(msg, "ssss")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;
    char *json = msg_argv(msg)[2];
    if (!broker_parse_hmm(&hmm, json)) return;
    char const *state = msg_argv(msg)[3];
    if (!strcmp(state, "done"))
        broker_send(SCHEDY_ID, fmt("job_inc_progress %lld 100", hmm.job_id));
    broker_send(SCHEDY_ID, fmt("job_set_state %lld %s", hmm.job_id, state));
}

static void fn_update_press_progress_1(struct msg *msg)
{
    if (msg_check(msg, "sss")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;
    if (msg_check(msg, "ssi")) return;
    int inc = (int)msg_int(msg, 2);
    if (inc <= 0) return;
    broker_send(PRESSY_ID,
                fmt("filename | update_press_progress_2 {1} {2} %d", inc));
}

static void fn_update_press_progress_2(struct msg *msg)
{
    if (msg_check(msg, "ss*")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;
    if (msg_check(msg, "sssi")) return;
    char const *filename = msg_argv(msg)[2];
    int inc = (int)msg_int(msg, 3);
    broker_send(
        SCHEDY_ID,
        fmt("hmm_get_by_filename %s | update_press_progress_3 {1} {2} %d",
            filename, inc));
}

static void fn_update_press_progress_3(struct msg *msg)
{
    if (msg_check(msg, "ss*")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;
    if (msg_check(msg, "sssi")) return;
    char *json = msg_argv(msg)[2];
    int inc = (int)msg_int(msg, 3);
    if (!broker_parse_hmm(&hmm, json)) return;
    broker_send(SCHEDY_ID, fmt("job_inc_progress %lld %d", hmm.job_id, inc));
}

static void fn_pressy_avail_answer(struct msg *msg)
{
    if (msg_check(msg, "sssi")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;
    if (strcmp(msg_argv(msg)[2], "idle")) return;

    int64_t job_id = msg_int(msg, 3);
    broker_send(SCHEDY_ID,
                fmt("job_set_state %lld run | exec_press_job_1 {1} %lld",
                    job_id, job_id));
}

static void fn_exec_press_job_1(struct msg *msg)
{
    if (msg_check(msg, "ssi")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;

    int64_t job_id = msg_int(msg, 2);
    broker_send(
        SCHEDY_ID,
        fmt("hmm_get_by_job_id %lld | exec_press_job_2 {1} {2}", job_id));
}

static void fn_exec_press_job_2(struct msg *msg)
{
    if (msg_check(msg, "sss")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;
    char *json = msg_argv(msg)[2];
    if (!broker_parse_hmm(&hmm, json)) return;
    sched_dump_hmm(&hmm, buffer);
    broker_send(SCHEDY_ID, fmt("hmm_dl %lld %s | exec_press_job_3 {1} %s",
                               hmm.xxh3, hmm.filename, buffer));
}

static void fn_exec_press_job_3(struct msg *msg)
{
    if (msg_check(msg, "sss")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;
    char *json = msg_argv(msg)[2];
    if (!broker_parse_hmm(&hmm, json)) return;
    broker_send(PRESSY_ID, fmt("press %s", hmm.filename));
}

#include "command.h"
#include "broker.h"
#include "core/as.h"
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
    X(SCHEDY_POLLING, schedy_polling, "TODO")                                  \
    X(SCANNY_POLLING, scanny_polling, "TODO")                                  \
    X(UPDATE_PRESS_PROGRESS_1, update_press_progress_1, "TODO")                \
    X(UPDATE_PRESS_PROGRESS_2, update_press_progress_2, "TODO")                \
    X(UPDATE_PRESS_PROGRESS_3, update_press_progress_3, "TODO")                \
    X(UPDATE_PRESS_STATE_1, update_press_state_1, "TODO")                      \
    X(UPDATE_PRESS_STATE_2, update_press_state_2, "TODO")                      \
    X(UPDATE_PRESS_STATE_3, update_press_state_3, "TODO")                      \
    X(EXEC_SCAN_JOB_1, exec_scan_job_1, "TODO")                                \
    X(EXEC_SCAN_JOB_2, exec_scan_job_2, "TODO")                                \
    X(EXEC_SCAN_JOB_3, exec_scan_job_3, "TODO")                                \
    X(EXEC_SCAN_JOB_4, exec_scan_job_4, "TODO")                                \
    X(EXEC_SCAN_JOB_5, exec_scan_job_5, "TODO")                                \
    X(EXEC_SCAN_JOB_6, exec_scan_job_6, "TODO")                                \
    X(UPDATE_SCAN_STATE_1, update_scan_state_1, "TODO")                        \
    X(UPDATE_SCAN_STATE_2, update_scan_state_2, "TODO")                        \
    X(UPDATE_SCAN_PROGRESS_1, update_scan_progress_1, "TODO")                  \
    X(UPDATE_SCAN_PROGRESS_2, update_scan_progress_2, "TODO")                  \
    X(EXEC_PRESS_JOB_1, exec_press_job_1, "TODO")                              \
    X(EXEC_PRESS_JOB_2, exec_press_job_2, "TODO")                              \
    X(EXEC_PRESS_JOB_3, exec_press_job_3, "TODO")                              \
    X(EXEC_PRESS_JOB_4, exec_press_job_4, "TODO")

#define COMMAND_TEMPLATE_DEF
#include "core/command_template.h"
#undef COMMAND_TEMPLATE_DEF

static struct sched_job job = {0};
static struct sched_hmm hmm = {0};
static struct sched_db db = {0};
static struct sched_scan scan = {0};
static char buffer[1024] = {0};

static int64_t filename_id(char *name)
{
    char *p = strchr(name, '_');
    if (!*p) return 0;
    *p = '\0';
    int64_t r = as_int64(name);
    *p = '_';
    return r;
}

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
    if (strcmp(msg_argv(msg)[1], "ok")) return;

    char *json = msg_argv(msg)[2];
    if (!broker_parse_job(&job, json)) return;

    enum sched_job_type type = sched_job_type(&job);

    if (type == SCHED_SCAN)
        broker_send(SCANNY_ID,
                    fmt("state | exec_scan_job_1 {1} {2} %lld", job.id));

    if (type == SCHED_HMM)
        broker_send(PRESSY_ID,
                    fmt("state | exec_press_job_1 {1} {2} %lld", job.id));
}

static void fn_scanny_polling(struct msg *msg)
{
    if (msg_check(msg, "sss")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;
    if (!strcmp(msg_argv(msg)[2], "idle")) return;

    if (!strcmp(msg_argv(msg)[2], "run"))
    {
        broker_send(SCANNY_ID,
                    fmt("inc_progress | update_scan_progress_1 {1} {2}"));
    }
    else
    {
        char const *state = msg_argv(msg)[2];
        broker_send(SCANNY_ID,
                    fmt("filename | update_scan_state_1 {1} {2} %s", state));
    }
}

static void fn_update_scan_state_1(struct msg *msg)
{
    if (msg_check(msg, "ssss")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;
    char const *filename = msg_argv(msg)[2];
    char const *state = msg_argv(msg)[3];
    broker_send(SCANNY_ID,
                fmt("reset | update_scan_state_2 {1} %s %s", filename, state));
}

static void fn_update_scan_state_2(struct msg *msg)
{
    if (msg_check(msg, "ssss")) return;
    char *filename = msg_argv(msg)[2];
    char const *state = msg_argv(msg)[3];
    long job_id = filename_id(filename);

    if (!strcmp(state, "done"))
    {
        broker_send(SCHEDY_ID, fmt("prods_file_up %s", filename));
        broker_send(SCHEDY_ID, fmt("job_inc_progress %ld 100", job_id));
    }
    broker_send(SCHEDY_ID, fmt("job_set_state %ld %s", job_id, state));
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
    broker_send(SCHEDY_ID,
                fmt("job_set_state %ld %s", (long)hmm.job_id, state));
    size_t n = strlen(hmm.filename);
    hmm.filename[n - 3] = 'd';
    hmm.filename[n - 2] = 'c';
    hmm.filename[n - 1] = 'p';
    broker_send(SCHEDY_ID, fmt("db_up %s", hmm.filename));
}

static void fn_update_scan_progress_1(struct msg *msg)
{
    if (msg_check(msg, "sss")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;
    if (msg_check(msg, "ssi")) return;
    int inc = (int)msg_int(msg, 2);
    if (inc <= 0) return;
    broker_send(SCANNY_ID,
                fmt("filename | update_scan_progress_2 {1} {2} %d", inc));
}

static void fn_update_scan_progress_2(struct msg *msg)
{
    if (msg_check(msg, "ss*")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;
    if (msg_check(msg, "sssi")) return;
    char *filename = msg_argv(msg)[2];
    int inc = (int)msg_int(msg, 3);
    long job_id = (long)filename_id(filename);
    broker_send(SCHEDY_ID, fmt("job_inc_progress %ld %d", job_id, inc));
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

static void fn_exec_scan_job_1(struct msg *msg)
{
    if (msg_check(msg, "sssi")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;
    if (strcmp(msg_argv(msg)[2], "idle")) return;

    long job_id = msg_int(msg, 3);
    broker_send(
        SCHEDY_ID,
        fmt("job_set_state %ld run | exec_scan_job_2 {1} %ld", job_id, job_id));
}

static void fn_exec_scan_job_2(struct msg *msg)
{
    if (msg_check(msg, "ssi")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;

    int64_t job_id = msg_int(msg, 2);
    broker_send(
        SCHEDY_ID,
        fmt("scan_get_by_job_id %lld | exec_scan_job_3 {1} {2}", job_id));
}

static void fn_exec_scan_job_3(struct msg *msg)
{
    if (msg_check(msg, "sss")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;
    char *json = msg_argv(msg)[2];
    if (!broker_parse_scan(&scan, json)) return;
    sched_dump_scan(&scan, buffer);
    broker_send(
        SCHEDY_ID,
        fmt("scan_dl_seqs %lld seqs_%lld.json | exec_scan_job_4 {1} {2} %s",
            scan.id, scan.id, buffer));
}

static void fn_exec_scan_job_4(struct msg *msg)
{
    if (msg_check(msg, "ssss")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;
    char const *seqs_file = msg_argv(msg)[2];
    char *json = msg_argv(msg)[3];
    if (!broker_parse_scan(&scan, json)) return;
    sched_dump_scan(&scan, buffer);
    broker_send(SCHEDY_ID,
                fmt("db_get_by_id %lld | exec_scan_job_5 {1} {2} %s %s",
                    scan.db_id, buffer, seqs_file));
}

static void fn_exec_scan_job_5(struct msg *msg)
{
    if (msg_check(msg, "sssss")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;
    if (!broker_parse_db(&db, msg_argv(msg)[2])) return;
    if (!broker_parse_scan(&scan, msg_argv(msg)[3])) return;
    sched_dump_scan(&scan, buffer);
    char *seqs_file = msg_argv(msg)[4];
    broker_send(SCHEDY_ID,
                fmt("db_dl %lld %s | exec_scan_job_6 {1} %s %s %s", db.xxh3,
                    db.filename, buffer, seqs_file, db.filename));
}

static void fn_exec_scan_job_6(struct msg *msg)
{
    if (msg_check(msg, "sssss")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;
    if (!broker_parse_scan(&scan, msg_argv(msg)[2])) return;
    char *seqs_file = msg_argv(msg)[3];
    char *db_file = msg_argv(msg)[4];
    broker_send(SCANNY_ID,
                fmt("scan %s %s %lld_prods.tsv %d %d", seqs_file, db_file,
                    scan.job_id, scan.multi_hits, scan.hmmer3_compat));
}

static void fn_exec_press_job_1(struct msg *msg)
{
    if (msg_check(msg, "sssi")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;
    if (strcmp(msg_argv(msg)[2], "idle")) return;

    long job_id = (long)msg_int(msg, 3);
    broker_send(SCHEDY_ID,
                fmt("job_set_state %ld run | exec_press_job_2 {1} %ld", job_id,
                    job_id));
}

static void fn_exec_press_job_2(struct msg *msg)
{
    if (msg_check(msg, "ssi")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;

    int64_t job_id = msg_int(msg, 2);
    broker_send(
        SCHEDY_ID,
        fmt("hmm_get_by_job_id %lld | exec_press_job_3 {1} {2}", job_id));
}

static void fn_exec_press_job_3(struct msg *msg)
{
    if (msg_check(msg, "sss")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;
    char *json = msg_argv(msg)[2];
    if (!broker_parse_hmm(&hmm, json)) return;
    sched_dump_hmm(&hmm, buffer);
    broker_send(SCHEDY_ID, fmt("hmm_dl %lld %s | exec_press_job_4 {1} %s",
                               hmm.xxh3, hmm.filename, buffer));
}

static void fn_exec_press_job_4(struct msg *msg)
{
    if (msg_check(msg, "sss")) return;
    if (strcmp(msg_argv(msg)[1], "ok")) return;
    char *json = msg_argv(msg)[2];
    if (!broker_parse_hmm(&hmm, json)) return;
    broker_send(PRESSY_ID, fmt("press %s", hmm.filename));
}

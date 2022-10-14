#include "broker.h"
#include "core/errmsg.h"
#include "core/global.h"
#include "core/logy.h"
#include "core/msg.h"
#include "core/rc.h"
#include "core/sched.h"
#include "core/strings.h"
#include "decy.h"
#include "jx.h"
#include "loop/child.h"
#include "strings.h"
#include <stdatomic.h>

static void on_pressy_exit(void) { global_terminate(); }
static void on_pressy_eof(void) {}
static void on_pressy_read_error(void) {}
static void on_pressy_write_error(void) {}
static void on_pressy_read(char *line) { input_forward(&input, line); }

static void on_schedy_exit(void) { global_terminate(); }
static void on_schedy_eof(void) {}
static void on_schedy_read_error(void) {}
static void on_schedy_write_error(void) {}
static void on_schedy_read(char *line) { input_forward(&input, line); }

static void on_scanny_exit(void) { global_terminate(); }
static void on_scanny_eof(void) {}
static void on_scanny_read_error(void) {}
static void on_scanny_write_error(void) {}
static void on_scanny_read(char *line) { input_forward(&input, line); }

static struct child proc[3] = {
    [PRESSY_ID] = {0}, [SCANNY_ID] = {0}, [SCHEDY_ID] = {0}};

static char const *proc_name[] = {
    [PRESSY_ID] = "pressy",
    [SCANNY_ID] = "scanny",
    [SCHEDY_ID] = "schedy",
};

static int proc_idx(char const *name);

static char *proc_args[][16] = {
    [PRESSY_ID] = {"./pressy", "--pid", "pressy.pid", NULL},
    [SCANNY_ID] = {"./scanny", "--pid", "scanny.pid", NULL},
    [SCHEDY_ID] = {"./schedy", "--pid", "schedy.pid", NULL},
};

static struct child_cb child_callbacks[] = {
    [PRESSY_ID] = {&on_pressy_exit, NULL},
    [SCANNY_ID] = {&on_scanny_exit, NULL},
    [SCHEDY_ID] = {&on_schedy_exit, NULL},
};

static struct input_cb input_callbacks[] = {
    [PRESSY_ID] = {&on_pressy_eof, &on_pressy_read_error, &on_pressy_read},
    [SCANNY_ID] = {&on_scanny_eof, &on_scanny_read_error, &on_scanny_read},
    [SCHEDY_ID] = {&on_schedy_eof, &on_schedy_read_error, &on_schedy_read},
};

static struct output_cb output_callbacks[] = {
    [PRESSY_ID] = {&on_pressy_write_error},
    [SCANNY_ID] = {&on_scanny_write_error},
    [SCHEDY_ID] = {&on_schedy_write_error},
};

static JR_DECLARE(json_parser, 128);
static struct uv_timer_s job_next_pend_timer = {0};
static atomic_bool no_job_next_pend = false;
static char errmsg[ERROR_SIZE] = {0};

static void job_next_pend_cb(struct uv_timer_s *req);

void broker_init(void)
{
    JR_INIT(json_parser);
    for (int i = 0; i <= SCHEDY_ID; ++i)
    {
        child_init(&proc[i]);
        child_cb(&proc[i])->on_exit = child_callbacks[i].on_exit;
        child_cb(&proc[i])->arg = child_callbacks[i].arg;
        child_input_cb(&proc[i])->on_eof = input_callbacks[i].on_eof;
        child_input_cb(&proc[i])->on_error = input_callbacks[i].on_error;
        child_input_cb(&proc[i])->on_read = input_callbacks[i].on_read;
        child_output_cb(&proc[i])->on_error = output_callbacks[i].on_error;
        child_spawn(&proc[i], proc_args[i]);
    }
    atomic_store_explicit(&no_job_next_pend, false, memory_order_release);

    if (uv_timer_init(global_loop(), &job_next_pend_timer))
        efail("uv_timer_init");
    if (uv_timer_start(&job_next_pend_timer, &job_next_pend_cb, 1000, 5000))
        efail("uv_timer_start");
}

static void job_next_pend_cb(struct uv_timer_s *req)
{
    (void)req;
    if (atomic_load_explicit(&no_job_next_pend, memory_order_consume)) return;
    debug("Asking for pending job");
    child_send(&proc[SCHEDY_ID], "job_next_pend | exec_pend_job {1} {2}");
}

char const *broker_forward_msg(char const *proc_name, struct msg *msg)
{
    if (!sharg_check(&msg->cmd, "s*")) return eparse(FAIL_PARSE), FAIL;
    int i = proc_idx(proc_name);
    if (i < 0) return FAIL;
    child_send(&proc[i], msg_unparse(msg));
    return OK;
}

void broker_terminate(void) { uv_timer_stop(&job_next_pend_timer); }

bool broker_parse_db(struct sched_db *db, char *json)
{
    if (jr_parse(json_parser, (int)strlen(json), json))
        return !eparse("%s", errfmt(errmsg, FAIL_PARSE_JSON));
    return !sched_db_parse(db, json_parser);
}

bool broker_parse_hmm(struct sched_hmm *hmm, char *json)
{
    if (jr_parse(json_parser, (int)strlen(json), json))
        return !eparse("%s", errfmt(errmsg, FAIL_PARSE_JSON));
    return !sched_hmm_parse(hmm, json_parser);
}

bool broker_parse_job(struct sched_job *job, char *json)
{
    if (jr_parse(json_parser, (int)strlen(json), json))
        return !eparse("%s", errfmt(errmsg, FAIL_PARSE_JSON));
    return !sched_job_parse(job, json_parser);
}

bool broker_parse_scan(struct sched_scan *scan, char *json)
{
    if (jr_parse(json_parser, (int)strlen(json), json))
        return !eparse("%s", errfmt(errmsg, FAIL_PARSE_JSON));
    return !sched_scan_parse(scan, json_parser);
}

bool broker_parse_seq(struct sched_seq *seq, char *json)
{
    if (jr_parse(json_parser, (int)strlen(json), json))
        return !eparse("%s", errfmt(errmsg, FAIL_PARSE_JSON));
    return !sched_seq_parse(seq, json_parser);
}

static int proc_idx(char const *name)
{
    for (int i = 0; i <= SCHEDY_ID; ++i)
        if (!strcmp(proc_name[i], name)) return i;
    return -1;
}

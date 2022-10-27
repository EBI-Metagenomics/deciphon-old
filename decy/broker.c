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
#include "loop/proc.h"
#include <stdatomic.h>
#include <string.h>

static enum proc_type proc_type[] = {[PARENT_ID] = PROC_PARENT,
                                     [PRESSY_ID] = PROC_CHILD,
                                     [SCANNY_ID] = PROC_CHILD,
                                     [SCHEDY_ID] = PROC_CHILD};

static struct proc proc[] = {
    [PARENT_ID] = {0}, [PRESSY_ID] = {0}, [SCANNY_ID] = {0}, [SCHEDY_ID] = {0}};

static char *proc_args[][16] = {
    [PARENT_ID] = {NULL},
    [PRESSY_ID] = {"./pressy", "--pid", "pressy.pid", NULL},
    [SCANNY_ID] = {"./scanny", "--pid", "scanny.pid", NULL},
    [SCHEDY_ID] = {"./schedy", "--pid", "schedy.pid", NULL},
};

static JR_DECLARE(json_parser, 128);
static struct uv_timer_s next_pend_job_timer = {0};
static uint64_t polling = 1000;
static atomic_bool no_job_next_pend = false;
static char errmsg[ERROR_SIZE] = {0};

static void next_pend_job(struct uv_timer_s *req);

void broker_init(int64_t repeat)
{
    JR_INIT(json_parser);
    polling = (uint64_t)repeat;

    for (int i = 0; i <= SCHEDY_ID; ++i)
    {
        proc_init(&proc[i], proc_type[i]);
        proc_setup(&proc[i], &on_read, &terminate, &terminate, &terminate);
    }
    for (int i = 0; i <= SCHEDY_ID; ++i)
        proc_start(&proc[i], proc_args[i]);

    atomic_store_explicit(&no_job_next_pend, false, memory_order_release);

    if (uv_timer_init(global_loop(), &next_pend_job_timer))
        efail("uv_timer_init");

    if (polling > 0)
    {
        if (uv_timer_start(&next_pend_job_timer, &next_pend_job, 1000, polling))
            efail("uv_timer_start");
    }
}

static void next_pend_job(struct uv_timer_s *req)
{
    (void)req;
    if (atomic_load_explicit(&no_job_next_pend, memory_order_consume)) return;
    debug("Asking for pending job");
    proc_send(&proc[SCHEDY_ID], "job_next_pend | exec_pend_job {1} {2}");
}

void broker_send(enum pid pid, char const *msg) { proc_send(&proc[pid], msg); }

void broker_terminate(void)
{
    if (polling > 0) uv_timer_stop(&next_pend_job_timer);
}

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

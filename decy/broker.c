#include "broker.h"
#include "command.h"
#include "core/errmsg.h"
#include "core/global.h"
#include "core/logy.h"
#include "core/msg.h"
#include "core/rc.h"
#include "core/sched.h"
#include "core/strings.h"
#include "jx.h"
#include "loop/proc.h"
#include "loop/timer.h"
#include <stdatomic.h>
#include <string.h>

static struct proc proc[] = {
    [PARENT_ID] = {0}, [PRESSY_ID] = {0}, [SCANNY_ID] = {0}, [SCHEDY_ID] = {0}};

static enum proc_type proc_type[] = {[PARENT_ID] = PROC_PARENT,
                                     [PRESSY_ID] = PROC_CHILD,
                                     [SCANNY_ID] = PROC_CHILD,
                                     [SCHEDY_ID] = PROC_CHILD};

static char const *proc_name[] = {"parent", "pressy", "scanny", "schedy"};

static char const *proc_args[][32] = {
    [PARENT_ID] = {NULL},
    [PRESSY_ID] = {"./pressy", "-p", "pressy.pid", NULL},
    [SCANNY_ID] = {"./scanny", "-p", "scanny.pid", NULL},
    [SCHEDY_ID] = {"./schedy", "-p", "schedy.pid", "-u", NULL, "-k", NULL,
                   NULL},
};

static JR_DECLARE(json_parser, 128);
static char errmsg[ERROR_SIZE] = {0};

static void polling_callback(void);
static void on_read(char *line);
static void terminate(void) { global_terminate(); }

static struct timer polling_timer = {0};

void broker_init(long polling, char const *uri, char const *key)
{
    JR_INIT(json_parser);
    timer_init(&polling_timer, polling, &polling_callback);
    proc_args[SCHEDY_ID][4] = uri;
    proc_args[SCHEDY_ID][6] = key;

    for (int i = 0; i <= SCHEDY_ID; ++i)
    {
        proc_init(&proc[i], proc_type[i]);
        proc_setup(&proc[i], &on_read, &terminate, &terminate, &terminate);
        proc_start(&proc[i], proc_args[i]);
    }
}

void broker_send(enum pid pid, char const *msg)
{
    debug("%s --> %s", msg, proc_name[pid]);
    proc_send(&proc[pid], msg);
}

int broker_resolve_procname(char const *name)
{
    for (int i = 0; i <= SCHEDY_ID; ++i)
    {
        if (!strcmp(proc_name[i], name)) return i;
    }
    error("unexpected process name <%s>", name);
    return -1;
}

void broker_terminate(void)
{
    timer_cleanup(&polling_timer);
    for (int i = 0; i <= SCHEDY_ID; ++i)
        proc_stop(&proc[i]);
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

static void polling_callback(void)
{
    static int cnt = 0;
    if (cnt % 2 == 0)
        proc_send(&proc[SCHEDY_ID], "job_next_pend | schedy_polling {1} {2}");
    if (cnt % 2 == 1)
        proc_send(&proc[PRESSY_ID], "state | pressy_polling {1} {2}");
    ++cnt;
}

static void on_read(char *line)
{
    static struct msg msg = {0};
    if (msg_parse(&msg, line)) return;

    cmd_fn_t *cmd_fn = cmd_get_fn(msg_cmd(&msg));
    if (!cmd_fn) return;

    (*cmd_fn)(&msg);
}

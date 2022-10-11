#include "decy/session.h"
#include "core/cmd.h"
#include "core/global.h"
#include "core/logy.h"
#include "core/pp.h"
#include "core/rc.h"
#include "decy/decy.h"
#include "decy/strings.h"
#include "loop/child.h"
#include <stdatomic.h>

static void on_pressy_exit(void) { global_terminate(); }
static void on_pressy_eof(void *arg) { UNUSED(arg); }
static void on_pressy_read_error(void *arg) { UNUSED(arg); }
static void on_pressy_write_error(void *arg) { UNUSED(arg); }
static void on_pressy_read(char *line, void *arg)
{
    UNUSED(arg);
    static char string[512] = {0};
    snprintf(string, sizeof string, "%s", line);
    output_put(&output, string);
}

static void on_schedy_exit(void) { global_terminate(); }
static void on_schedy_eof(void *arg) { UNUSED(arg); }
static void on_schedy_read_error(void *arg) { UNUSED(arg); }
static void on_schedy_write_error(void *arg) { UNUSED(arg); }
static void on_schedy_read(char *line, void *arg)
{
    UNUSED(arg);
    static char string[512] = {0};
    snprintf(string, sizeof string, "%s", line);
    output_put(&output, string);
}

static void on_scanny_exit(void) { global_terminate(); }
static void on_scanny_eof(void *arg) { UNUSED(arg); }
static void on_scanny_read_error(void *arg) { UNUSED(arg); }
static void on_scanny_write_error(void *arg) { UNUSED(arg); }
static void on_scanny_read(char *line, void *arg)
{
    UNUSED(arg);
    static char string[512] = {0};
    snprintf(string, sizeof string, "%s", line);
    output_put(&output, string);
}

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
    [PRESSY_ID] = {&on_pressy_eof, &on_pressy_read_error, &on_pressy_read,
                   NULL},
    [SCANNY_ID] = {&on_scanny_eof, &on_scanny_read_error, &on_scanny_read,
                   NULL},
    [SCHEDY_ID] = {&on_schedy_eof, &on_schedy_read_error, &on_schedy_read,
                   NULL},
};

static struct output_cb output_callbacks[] = {
    [PRESSY_ID] = {&on_pressy_write_error, NULL},
    [SCANNY_ID] = {&on_scanny_write_error, NULL},
    [SCHEDY_ID] = {&on_schedy_write_error, NULL},
};

struct uv_timer_s job_next_pend_timer = {0};
atomic_bool no_job_next_pend = false;

static void job_next_pend_cb(struct uv_timer_s *req);

void session_init(void)
{
    for (int i = 0; i <= SCHEDY_ID; ++i)
    {
        child_init(&proc[i]);
        child_cb(&proc[i])->on_exit = child_callbacks[i].on_exit;
        child_cb(&proc[i])->arg = child_callbacks[i].arg;
        child_input_cb(&proc[i])->on_eof = input_callbacks[i].on_eof;
        child_input_cb(&proc[i])->on_error = input_callbacks[i].on_error;
        child_input_cb(&proc[i])->on_read = input_callbacks[i].on_read;
        child_input_cb(&proc[i])->arg = input_callbacks[i].arg;
        child_output_cb(&proc[i])->on_error = output_callbacks[i].on_error;
        child_output_cb(&proc[i])->arg = output_callbacks[i].arg;
        child_spawn(&proc[i], proc_args[i]);
    }
    atomic_store_explicit(&no_job_next_pend, false, memory_order_release);

    if (uv_timer_init(global_loop(), &job_next_pend_timer))
        efail("uv_timer_init");
    if (uv_timer_start(&job_next_pend_timer, &job_next_pend_cb, 2000, 1000))
        efail("uv_timer_start");
}

static void job_next_pend_cb(struct uv_timer_s *req)
{
    (void)req;
    if (atomic_load_explicit(&no_job_next_pend, memory_order_consume)) return;
    debug("Asking for pending job");
    child_send(&proc[SCHEDY_ID], "job_next_pend");
}

char const *session_forward_command(char const *proc_name, struct cmd *cmd)
{
    if (!cmd_check(cmd, "s*")) return eparse(FAIL_PARSE), FAIL;
    int i = proc_idx(proc_name);
    if (i < 0) return FAIL;
    child_send(&proc[i], cmd_unparse(cmd));
    return OK;
}

void session_terminate(void) { uv_timer_stop(&job_next_pend_timer); }

static int proc_idx(char const *name)
{
    for (int i = 0; i <= SCHEDY_ID; ++i)
        if (!strcmp(proc_name[i], name)) return i;
    return -1;
}

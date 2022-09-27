#include "decy_schedy.h"
#include "core/c23.h"
#include "core/logging.h"
#include "core/pp.h"
#include "loop/ipc.h"
#include "uv.h"
#include <stdatomic.h>
#include <unistd.h>

#define PROGRAM "schedy"
static char *args[] = {"./" PROGRAM, "--syslog=/dev/null",
                       "--userlog=schedy_user.log", nullptr};
static struct uv_loop_s *loop = nullptr;
static struct uv_process_s request = {0};
static struct uv_process_options_s opts = {0};
static struct ipc ipc = {0};
static decy_schedy_onterm_fn_t *onterm_cb = nullptr;
static decy_schedy_onreply_fn_t *onreply_cb = nullptr;
static void *arg = nullptr;

static void onclose_process(struct uv_handle_s *handle);
static void close_process(struct uv_process_s *, int64_t exit_status,
                          int signal);
static void oneof(void *);
static void onerror(void *);
static void onread(char *line, void *);
static void onterm(void *);

void decy_schedy_init(struct uv_loop_s *loop_,
                      decy_schedy_onterm_fn_t *onterm_cb_, void *arg_)
{
    loop = loop_;
    ipc_init(&ipc, loop, onread, oneof, onerror, onterm, nullptr);
    onterm_cb = onterm_cb_;
    arg = arg_;

    opts.stdio = ipc_stdio(&ipc);
    opts.stdio_count = ipc_stdio_count(&ipc);
    opts.exit_cb = close_process;
    opts.file = args[0];
    opts.args = args;

    int r = 0;
    if ((r = uv_spawn(loop, &request, &opts)))
        fatal("Failed to launch " PROGRAM ": %s", uv_strerror(r));
    else
        info("Launched " PROGRAM " with ID %d", request.pid);

    ipc_start_reading(&ipc);
}

void decy_schedy_connect(char const *msg, decy_schedy_onreply_fn_t *onreply_cb_)
{
    onreply_cb = onreply_cb_;
    ipc_put(&ipc, msg);
}

void decy_schedy_terminate(void) { ipc_terminate(&ipc); }

static void close_process(uv_process_t *req, int64_t exit_status, int signal)
{
    (void)signal;
    info("Process %d exited with status %d", req->pid, (int)exit_status);
    uv_close((uv_handle_t *)req, onclose_process);
}

static void onclose_process(struct uv_handle_s *handle) { (void)handle; }

static void oneof(void *arg_)
{
    (void)arg_;
    ipc_terminate(&ipc);
}

static void onerror(void *arg_)
{
    (void)arg_;
    ipc_terminate(&ipc);
}

static void onread(char *line, void *arg_)
{
    (void)arg_;
    if (onreply_cb)
        (*onreply_cb)(line, arg_);
    else
        warn("ignored reply: %s", line);
}

static void onterm(void *arg_)
{
    (void)arg_;
    info("%s", __FUNCTION__);
    int r = uv_process_kill(&request, SIGTERM);
    if (r) efail("Failed to kill " PROGRAM ": %s", uv_strerror(r));
    (*onterm_cb)(arg);
}

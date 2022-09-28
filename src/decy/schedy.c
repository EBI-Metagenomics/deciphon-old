#include "decy/schedy.h"
#include "core/c23.h"
#include "core/logging.h"
#include "core/pp.h"
#include "loop/ipc.h"
#include "uv.h"
#include <stdatomic.h>
#include <unistd.h>

#define PROGRAM "schedy"

static struct
{
    char *args[4];
    struct uv_loop_s *loop;
    struct uv_process_s request;
    struct uv_process_options_s opts;
    struct ipc ipc;
    bool terminated;
    onterm_fn_t *onterm_cb;
    onreply_fn_t *onreply_cb;
    void *cb_arg;
} self = {
    .args = {"./" PROGRAM, "--syslog=/dev/null", "--userlog=schedy_user.log",
             nullptr},
    .loop = nullptr,
    .request = {0},
    .opts = {0},
    .ipc = {0},
    .terminated = false,
    .onterm_cb = nullptr,
    .onreply_cb = nullptr,
    .cb_arg = nullptr,
};

static void onclose_process(struct uv_handle_s *handle);
static void close_process(struct uv_process_s *, int64_t exit_status,
                          int signal);
static void oneof(void *);
static void onerror(void *);
static void onread(char *line, void *);
static void onterm(void *);

void schedy_init(struct uv_loop_s *loop, onterm_fn_t *onterm_cb_, void *arg)
{
    self.loop = loop;
    ipc_init(&self.ipc, self.loop, onread, oneof, onerror, onterm, nullptr);
    self.onterm_cb = onterm_cb_;
    self.cb_arg = arg;

    self.opts.stdio = ipc_stdio(&self.ipc);
    self.opts.stdio_count = ipc_stdio_count(&self.ipc);
    self.opts.exit_cb = close_process;
    self.opts.file = self.args[0];
    self.opts.args = self.args;

    int r = 0;
    if ((r = uv_spawn(self.loop, &self.request, &self.opts)))
        fatal("Failed to launch " PROGRAM ": %s", uv_strerror(r));
    else
        info("Launched " PROGRAM " with ID %d", self.request.pid);

    ipc_start_reading(&self.ipc);
}

void schedy_setup(char const *msg, onreply_fn_t *onreply_cb)
{
    self.onreply_cb = onreply_cb;
    ipc_put(&self.ipc, msg);
}

void schedy_is_online(onreply_fn_t *onreply_cb)
{
    self.onreply_cb = onreply_cb;
    ipc_put(&self.ipc, "online");
}

void schedy_terminate(void)
{
    info("%s", __FUNCTION__);
    ipc_terminate(&self.ipc);
}

bool schedy_isterminated(void) { return self.terminated; }

static void close_process(uv_process_t *req, int64_t exit_status, int signal)
{
    (void)signal;
    info("Process %d exited with status %d", req->pid, (int)exit_status);
    uv_close((uv_handle_t *)req, onclose_process);
}

static void onclose_process(struct uv_handle_s *handle) { (void)handle; }

static void oneof(void *arg)
{
    (void)arg;
    ipc_terminate(&self.ipc);
}

static void onerror(void *arg)
{
    (void)arg;
    ipc_terminate(&self.ipc);
}

static void onread(char *line, void *arg)
{
    (void)arg;
    if (self.onreply_cb)
        (*self.onreply_cb)(line, arg);
    else
        warn("ignored reply: %s", line);
}

static void onterm(void *arg)
{
    (void)arg;
    info("%s", __FUNCTION__);
    int r = uv_process_kill(&self.request, SIGTERM);
    if (r) efail("Failed to kill " PROGRAM ": %s", uv_strerror(r));
    self.terminated = true;
    (*self.onterm_cb)(self.cb_arg);
}

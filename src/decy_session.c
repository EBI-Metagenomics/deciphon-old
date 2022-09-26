#include "decy_session.h"
#include "core/c23.h"
#include "core/logging.h"
#include "core/pp.h"
#include "uv.h"
#include <inttypes.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

static struct uv_loop_s *loop = nullptr;
static atomic_bool cancel = false;

static struct
{
    char *args[3];
    struct uv_process_s request;
    struct uv_process_options_s opts;
    struct uv_pipe_s opipe;
    struct uv_pipe_s ipipe;

    struct uv_stdio_container_s stdio[3];
} schedy = {.args = {"./schedy", "--syslog=/dev/null", nullptr}};

static void schedy_init(void);
static void schedy_cleanup(void);
static void close_process_handle(struct uv_process_s *, int64_t exit_status,
                                 int term_signal);

void decy_session_init(struct uv_loop_s *loop_)
{
    loop = loop_;
    cancel = false;
    schedy_init();
}

void decy_session_cleanup(void) { schedy_cleanup(); }

static void schedy_init(void)
{
    schedy.opts.stdio_count = ARRAY_SIZE(schedy.stdio);
    uv_pipe_init(loop, &schedy.opipe, 1);
    uv_pipe_init(loop, &schedy.ipipe, 1);

    // uv_write2(write_req, (uv_stream_t*) &worker->pipe, &dummy_buf, 1,
    // (uv_stream_t*) client, nullptr);
    schedy.stdio[0].flags = UV_CREATE_PIPE | UV_WRITABLE_PIPE;
    schedy.stdio[0].data.stream = (struct uv_stream_s *)&schedy.opipe;

    schedy.stdio[1].flags = UV_CREATE_PIPE | UV_READABLE_PIPE;
    schedy.stdio[1].data.stream = (struct uv_stream_s *)&schedy.ipipe;

    schedy.stdio[2].flags = UV_INHERIT_FD;
    schedy.stdio[2].data.fd = STDERR_FILENO;

    schedy.opts.stdio = schedy.stdio;
    schedy.opts.stdio_count = ARRAY_SIZE(schedy.stdio);
    schedy.opts.exit_cb = close_process_handle;
    schedy.opts.file = schedy.args[0];
    schedy.opts.args = schedy.args;

    int r = 0;
    if ((r = uv_spawn(loop, &schedy.request, &schedy.opts)))
    {
        error("Failed to launch schedy: %s", uv_strerror(r));
        exit(0);
    }
    else
        info("Launched schedy with ID %d", schedy.request.pid);
}

static void schedy_cleanup(void)
{
    int r = uv_process_kill(&schedy.request, SIGTERM);
    if (r) error("Failed to kill schedy: %s", uv_strerror(r));
}

static void close_process_handle(uv_process_t *req, int64_t exit_status,
                                 int term_signal)
{
    info("Process %d exited with status %" PRId64 ", signal %d",
         schedy.request.pid, exit_status, term_signal);
    uv_close((uv_handle_t *)req, nullptr);
}

#include "core/c23.h"
#include "core/logging.h"
#include "core/pp.h"
#include "loop/reader.h"
#include "loop/writer.h"
#include "uv.h"
#include <stdatomic.h>
#include <unistd.h>

static char *args[3] = {"./schedy", "--syslog=/dev/null", nullptr};

static struct uv_loop_s *loop = nullptr;
static struct uv_process_s request = {0};
static struct uv_process_options_s opts = {0};
static struct uv_stdio_container_s stdio[3];

static atomic_bool cancel = false;

static struct writer writer = {0};
static struct reader reader = {0};

static void onclose_process(struct uv_handle_s *handle);
static void close_process(struct uv_process_s *, int64_t exit_status,
                          int signal);
static void writer_onclose(void *arg);

void decy_schedy_init(void)
{
    writer_init(&writer, loop, writer_onclose, nullptr);
    reader_init(&reader, loop, schedy_reader_onerror, schedy_reader_onread,
                &schedy);

    uv_pipe_init(loop, writer_pipe(&writer), 0);
    uv_pipe_init(loop, &ipipe, 0);

    stdio[0].flags = UV_CREATE_PIPE | UV_READABLE_PIPE;
    stdio[0].data.stream = (struct uv_stream_s *)writer_pipe(&writer);

    stdio[1].flags = UV_CREATE_PIPE | UV_WRITABLE_PIPE;
    stdio[1].data.stream = (struct uv_stream_s *)&ipipe;

    stdio[2].flags = UV_INHERIT_FD;
    stdio[2].data.fd = STDERR_FILENO;

    opts.stdio = stdio;
    opts.stdio_count = ARRAY_SIZE(stdio);
    opts.exit_cb = close_process;
    opts.file = args[0];
    opts.args = args;

    int r = 0;
    if ((r = uv_spawn(loop, &request, &opts)))
        fatal("Failed to launch schedy: %s", uv_strerror(r));
    else
        info("Launched schedy with ID %d", request.pid);
}

static void close_process(uv_process_t *req, int64_t exit_status, int signal)
{
    (void)signal;
    int pid = request.pid;
    info("Process %d exited with status %d", pid, (int)exit_status);
    uv_close((uv_handle_t *)req, onclose_process);
}

static void onclose_process(struct uv_handle_s *handle)
{
    (void)handle;
    writer_close(&writer);
    reader_close(&reader);
}

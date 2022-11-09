#include "loop/child.h"
#include "core/global.h"
#include "core/logy.h"
#include <unistd.h>

void child_init(struct child *child, on_read2_fn_t *on_read,
                on_eof2_fn_t *on_eof, on_error2_fn_t *on_error,
                on_exit_fn_t *on_exit)
{
    input_init(&child->input, -1);
    output_init(&child->output, -1);
    child->input.cb.on_read = on_read;
    child->input.cb.on_eof = on_eof;
    child->input.cb.on_error = on_error;
    child->output.cb.on_error = on_error;
    child->on_exit = on_exit;
    child->proc.data = child;

    child->stdio[0].flags = UV_CREATE_PIPE | UV_READABLE_PIPE;
    child->stdio[0].data.stream = (uv_stream_t *)&child->output.pipe;

    child->stdio[1].flags = UV_CREATE_PIPE | UV_WRITABLE_PIPE;
    child->stdio[1].data.stream = (uv_stream_t *)&child->input.pipe;

    child->stdio[2].flags = UV_INHERIT_FD;
    child->stdio[2].data.fd = STDERR_FILENO;

    child->alive = false;
    child->exit_status = 0;
}

static void on_exit_fwd(uv_process_t *proc, int64_t exit_status, int sig);

void child_spawn(struct child *child, char const *args[])
{
    child->opts.stdio = child->stdio;
    child->opts.stdio_count = 3;
    child->opts.exit_cb = on_exit_fwd;
    child->opts.file = args[0];
    child->opts.args = (char **)args;

    if (uv_spawn(global_loop(), &child->proc, &child->opts)) fatal("uv_spawn");
    child->alive = true;

    input_start(&child->input);
    output_start(&child->output);
}

void child_send(struct child *child, char const *string)
{
    if (string) writer_put(&child->output.writer, string);
}

void child_kill(struct child *child)
{
    if (!child->alive) return;
    uv_pid_t pid = uv_process_get_pid(&child->proc);
    int rc = uv_kill(pid, SIGINT);
    if (ESRCH == -rc)
        debug("uv_kill: %s", uv_strerror(rc));
    else if (rc)
        fatal("uv_kill %d: %s", rc, uv_strerror(rc));
    child->alive = false;
}

int child_exit_status(struct child const *child) { return child->exit_status; }

static void on_exit_fwd(uv_process_t *proc, int64_t exit_status, int sig)
{
    int rc = (int)exit_status;
    debug("Process exited with status %d, signal %d", rc, sig);
    struct child *child = proc->data;
    input_close(&child->input);
    output_close(&child->output);
    uv_close((uv_handle_t *)proc, NULL);
    child->exit_status = rc;
    (*child->on_exit)();
}

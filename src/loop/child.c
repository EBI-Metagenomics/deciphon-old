#define _POSIX_C_SOURCE 200809L
#include "loop/child.h"
#include "die.h"
#include "logy.h"
#include "loop/global.h"
#include "loop/reader.h"
#include "loop/writer.h"
#include <unistd.h>
#include <uv.h>

struct child
{
    struct
    {
        struct reader rdr;
        struct uv_pipe_s pipe;
    } in;

    struct
    {
        struct writer wrt;
        struct uv_pipe_s pipe;
    } out;

    struct uv_process_s proc;
    struct uv_process_options_s opts;
    struct uv_stdio_container_s stdio[3];

    void *userdata;
    on_exit4_fn_t *on_exit;
    bool no_kill;
    bool closed;
};

static void on_proc_exit(uv_process_t *proc, int64_t exit_status, int sig);

struct child *child_new(on_read2_fn_t *on_read, on_eof2_fn_t *on_eof,
                        on_error2_fn_t *on_error, on_exit4_fn_t *on_exit,
                        void *userdata)
{
    struct child *child = malloc(sizeof(*child));
    if (!child) die();

    if (uv_pipe_init(global_loop(), &child->in.pipe, 0)) die();
    if (uv_pipe_init(global_loop(), &child->out.pipe, 0)) die();

    reader_init(&child->in.rdr, &child->in.pipe, on_read, on_eof, on_error);
    writer_init(&child->out.wrt, &child->out.pipe, on_error);

    child->on_exit = on_exit;
    child->userdata = userdata;
    child->proc.data = child;

    child->stdio[0].flags = UV_CREATE_PIPE | UV_READABLE_PIPE;
    child->stdio[0].data.stream = (uv_stream_t *)&child->out.pipe;

    child->stdio[1].flags = UV_CREATE_PIPE | UV_WRITABLE_PIPE;
    child->stdio[1].data.stream = (uv_stream_t *)&child->in.pipe;

    child->stdio[2].flags = UV_INHERIT_FD;
    child->stdio[2].data.fd = STDERR_FILENO;

    child->no_kill = true;
    child->closed = true;
    return child;
}

void child_spawn(struct child *child, char const *args[])
{
    child->opts.stdio = child->stdio;
    child->opts.stdio_count = 3;
    child->opts.exit_cb = &on_proc_exit;
    child->opts.file = args[0];
    child->opts.args = (char **)args;

    if (uv_spawn(global_loop(), &child->proc, &child->opts)) fatal("uv_spawn");

    child->no_kill = false;
    child->closed = false;

    reader_open(&child->in.rdr);
}

void child_send(struct child *child, char const *string)
{
    if (string) writer_put(&child->out.wrt, string);
}

void child_kill(struct child *child)
{
    if (child->no_kill) return;
    child->no_kill = true;

    uv_pid_t pid = uv_process_get_pid(&child->proc);
    int rc = uv_kill(pid, SIGINT);
    if (ESRCH == -rc)
        debug("uv_kill: %s", uv_strerror(rc));
    else if (rc)
        fatal("uv_kill %d: %s", rc, uv_strerror(rc));
}

bool child_closed(struct child const *child) { return child->closed; }

void child_close(struct child *child)
{
    if (child->closed) return;
    child->no_kill = true;
    child->closed = true;
    reader_close(&child->in.rdr);
    writer_close(&child->out.wrt);
    uv_close((uv_handle_t *)&child->proc, NULL);
}

void child_del(struct child *child) { free(child); }

static void on_proc_exit(uv_process_t *proc, int64_t exit_status, int sig)
{
    int rc = (int)exit_status;
    debug("Process exited with status %d, signal %d", rc, sig);

    struct child *child = proc->data;
    child_close(child);

    if (child->on_exit) (*child->on_exit)(rc, child->userdata);
}

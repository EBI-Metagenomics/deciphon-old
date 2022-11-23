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

    void (*on_exit)(int, void *);
    void *callb_arg;

    bool no_reader;
    bool no_writer;

    bool no_kill;
    bool no_close;

    bool auto_delete;

    bool closed;
};

static void on_proc_exit(uv_process_t *proc, int64_t exit_status, int sig);

struct child *child_new(void)
{
    struct child *child = calloc(1, sizeof(*child));
    if (!child) NULL;

    child->proc.data = child;

    child->stdio[0].flags = UV_IGNORE;
    child->stdio[1].flags = UV_IGNORE;
    child->stdio[2].flags = UV_IGNORE;

    child->no_reader = true;
    child->no_writer = true;

    child->no_kill = true;
    child->no_close = false;

    child->auto_delete = false;

    child->closed = true;

    return child;
}

void child_enable_stdin(struct child *child, void (*error)())
{
    if (uv_pipe_init(global_loop(), &child->out.pipe, 0)) die();
    child->out.pipe.data = NULL;

    writer_init(&child->out.wrt, &child->out.pipe, error);

    child->stdio[0].flags = UV_CREATE_PIPE | UV_READABLE_PIPE;
    child->stdio[0].data.stream = (uv_stream_t *)&child->out.pipe;

    child->no_writer = false;
}

void child_enable_stdout(struct child *child, void (*read)(char *),
                         void (*eof)(), void (*error)())
{
    if (uv_pipe_init(global_loop(), &child->in.pipe, 0)) die();
    child->in.pipe.data = NULL;

    reader_init(&child->in.rdr, &child->in.pipe, read, eof, error);

    child->stdio[1].flags = UV_CREATE_PIPE | UV_WRITABLE_PIPE;
    child->stdio[1].data.stream = (uv_stream_t *)&child->in.pipe;

    child->no_reader = false;
}

void child_enable_stderr(struct child *child)
{
    child->stdio[2].flags = UV_INHERIT_FD;
    child->stdio[2].data.fd = STDERR_FILENO;
}

void child_set_on_exit(struct child *child, void (*on_exit)(int, void *))
{
    child->on_exit = on_exit;
}

void child_set_callb_arg(struct child *child, void *arg)
{
    child->callb_arg = arg;
}

void child_set_auto_delete(struct child *child, bool auto_delete)
{
    child->auto_delete = auto_delete;
}

bool child_spawn(struct child *child, char const *args[])
{
    child->opts.stdio = child->stdio;
    child->opts.stdio_count = 3;
    child->opts.exit_cb = &on_proc_exit;
    child->opts.file = args[0];
    child->opts.args = (char **)args;

    int rc = uv_spawn(global_loop(), &child->proc, &child->opts);
    if (rc) fatal("child_spawn %d: %s", rc, uv_strerror(rc));

    child->no_kill = false;
    child->no_close = false;
    child->closed = false;

    if (!child->no_reader) return reader_open(&child->in.rdr);
    return true;
}

void child_send(struct child *child, char const *string)
{
    if (string && !child->no_writer) writer_put(&child->out.wrt, string);
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

static void on_close(struct uv_handle_s *);

void child_close(struct child *child)
{
    if (child->no_close) return;
    child->no_close = true;
    child->no_kill = true;
    if (!child->no_reader) reader_close(&child->in.rdr);
    if (!child->no_writer) writer_close(&child->out.wrt);
    uv_close((uv_handle_t *)&child->proc, &on_close);
}

void child_del(struct child *child) { free(child); }

static void on_proc_exit(uv_process_t *proc, int64_t exit_status, int sig)
{
    int rc = (int)exit_status;
    debug("process exited with status %d, signal %d", rc, sig);

    struct child *child = proc->data;
    child_close(child);

    if (child->on_exit) (*child->on_exit)(rc, child->callb_arg);
}

static void on_close(struct uv_handle_s *h)
{
    struct child *child = h->data;
    if (child->auto_delete)
        child_del(child);
    else
        child->closed = true;
}

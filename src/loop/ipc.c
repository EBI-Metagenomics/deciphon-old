#include "loop/ipc.h"
#include "core/logy.h"
#include "core/pp.h"
#include <unistd.h>

static void reader_onclose(void *arg);
static void reader_oneof(void *arg);
static void reader_onerror(void *arg);
static void reader_onread(char *line, void *arg);
static void writer_onerror(void *arg);
static void writer_onclose(void *arg);
static void try_call_user_onterm(struct ipc *);

void ipc_init(struct ipc *ipc, struct uv_loop_s *loop,
              ipc_onread_fn_t *onread_cb, ipc_oneof_fn_t *oneof_cb,
              ipc_onerror_fn_t *onerror_cb, ipc_onterm_fn_t *onterm_cb,
              void *arg)
{
    ipc->loop = loop;
    reader_init(&ipc->reader, loop, 1, &reader_oneof, &reader_onerror,
                &reader_onread, &reader_onclose, ipc);
    writer_init(&ipc->writer, loop, 1, &writer_onerror, &writer_onclose, ipc);
    ipc->onread_cb = onread_cb;
    ipc->oneof_cb = oneof_cb;
    ipc->onerror_cb = onerror_cb;
    ipc->onterm_cb = onterm_cb;
    ipc->arg = arg;

    ipc->stdio[0].flags = UV_CREATE_PIPE | UV_READABLE_PIPE;
    ipc->stdio[0].data.stream = (struct uv_stream_s *)writer_pipe(&ipc->writer);

    ipc->stdio[1].flags = UV_CREATE_PIPE | UV_WRITABLE_PIPE;
    ipc->stdio[1].data.stream = (struct uv_stream_s *)reader_pipe(&ipc->reader);

    ipc->stdio[2].flags = UV_INHERIT_FD;
    ipc->stdio[2].data.fd = STDERR_FILENO;
}

void ipc_put(struct ipc *ipc, char const *msg)
{
    writer_put(&ipc->writer, msg);
}

struct uv_stdio_container_s *ipc_stdio(struct ipc *ipc) { return ipc->stdio; }

int ipc_stdio_count(struct ipc const *ipc) { return ARRAY_SIZE(ipc->stdio); }

void ipc_start_reading(struct ipc *ipc) { reader_start(&ipc->reader); }

void ipc_terminate(struct ipc *ipc)
{
    reader_close(&ipc->reader);
    writer_close(&ipc->writer);
}

static void reader_onclose(void *arg)
{
    struct ipc *ipc = arg;
    try_call_user_onterm(ipc);
}

static void reader_oneof(void *arg)
{
    struct ipc *ipc = arg;
    (*ipc->oneof_cb)(ipc->arg);
}

static void reader_onerror(void *arg)
{
    struct ipc *ipc = arg;
    (*ipc->onerror_cb)(ipc->arg);
}

static void reader_onread(char *line, void *arg)
{
    struct ipc *ipc = arg;
    (*ipc->onread_cb)(line, ipc->arg);
}

static void writer_onerror(void *arg)
{
    struct ipc *ipc = arg;
    (*ipc->onerror_cb)(ipc->arg);
}

static void writer_onclose(void *arg)
{
    struct ipc *ipc = arg;
    try_call_user_onterm(ipc);
}

static void try_call_user_onterm(struct ipc *ipc)
{
    if (reader_isclosed(&ipc->reader) && writer_isclosed(&ipc->writer))
        (*ipc->onterm_cb)(ipc->arg);
}

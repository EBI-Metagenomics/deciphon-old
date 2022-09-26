#include "loop/ipc.h"
#include "core/logging.h"
#include <unistd.h>

static void reader_onclose(void *arg);
static void reader_onerror(void *arg);
static void reader_onread(char *line, void *arg);
static void writer_onclose(void *arg);

void ipc_init(struct ipc *ipc, struct uv_loop_s *loop,
              ipc_onread_fn_t *onread_cb, void *arg)
{
    ipc->loop = loop;
    reader_init(&ipc->reader, loop, &reader_onerror, &reader_onread,
                &reader_onclose, ipc);
    writer_init(&ipc->writer, loop, &writer_onclose, ipc);
    ipc->onread_cb = onread_cb;
    ipc->arg = arg;
    ipc->reader_noclose = true;
    ipc->writer_noclose = true;

    ipc->stdio[0].flags = UV_CREATE_PIPE | UV_READABLE_PIPE;
    ipc->stdio[0].data.stream = (struct uv_stream_s *)&ipc->writer.pipe;

    ipc->stdio[1].flags = UV_CREATE_PIPE | UV_WRITABLE_PIPE;
    ipc->stdio[1].data.stream = (struct uv_stream_s *)&ipc->reader.pipe;

    ipc->stdio[2].flags = UV_INHERIT_FD;
    ipc->stdio[2].data.fd = STDERR_FILENO;
}

void ipc_terminate(struct ipc *ipc)
{
    if (!ipc->reader_noclose) reader_close(&ipc->reader);
    if (!ipc->writer_noclose) writer_close(&ipc->writer);
}

static void reader_onclose(void *arg)
{
    struct ipc *ipc = arg;
    io_close(&ipc->input);
}

static void reader_onerror(void *arg)
{
    eio("read error");
    struct ipc *ipc = arg;
    looper_terminate(l->looper);
}

static void reader_onread(char *line, void *arg)
{
    struct ipc *ipc = arg;
    (*ipc->onread_cb)(line, ipc->arg);
}

static void writer_onclose(void *arg)
{
    struct ipc *ipc = arg;
    io_close(&ipc->output);
}

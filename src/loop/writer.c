#include "loop/writer.h"
#include "core/logy.h"
#include "uv.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

struct request
{
    struct writer *writer;
    struct uv_write_s req;
    char const *msg;
};

void writer_init(struct writer *writer, struct uv_loop_s *loop, int ipc,
                 writer_onerror_fn_t *onerror_cb,
                 writer_onclose_fn_t *onclose_cb, void *arg)
{
    writer->loop = loop;
    writer->onerror_cb = onerror_cb;
    writer->onclose_cb = onclose_cb;
    writer->arg = arg;
    writer->closed = false;
    uv_pipe_init(writer->loop, &writer->pipe, ipc);
    ((struct uv_handle_s *)(&writer->pipe))->data = writer;
    ((struct uv_stream_s *)(&writer->pipe))->data = writer;
}

void writer_fopen(struct writer *writer, int fd)
{
    int rc = uv_pipe_open(&writer->pipe, fd);
    if (rc) fatal("%s", uv_strerror(rc));
    writer->closed = false;
}

static void *memdup(const void *mem, size_t size)
{
    void *out = malloc(size);
    if (!out) fatal("out of memory");
    memcpy(out, mem, size);
    return out;
}

static void write_cb(struct uv_write_s *write, int status);

void writer_put(struct writer *writer, char const *msg)
{
    unsigned size = (unsigned)strlen(msg);
    struct request *request = malloc(sizeof(struct request));
    request->writer = writer;
    request->msg = memdup(msg, size);
    request->req.data = request;

    struct uv_stream_s *stream = (struct uv_stream_s *)&writer->pipe;
    uv_buf_t bufs[2] = {uv_buf_init((char *)request->msg, size),
                        uv_buf_init((char *)"\n", 1)};

    int rc = uv_write(&request->req, stream, bufs, 2, &write_cb);
    if (rc)
    {
        eio("%s", uv_strerror(rc));
        (*writer->onerror_cb)(writer->arg);
    }
}

struct uv_pipe_s *writer_pipe(struct writer *writer) { return &writer->pipe; }

void onclose_cb(struct uv_handle_s *handle)
{
    struct writer *writer = handle->data;
    writer->closed = true;
    (*writer->onclose_cb)(writer->arg);
}

void writer_close(struct writer *writer)
{
    assert(!writer->closed);
    uv_close((struct uv_handle_s *)&writer->pipe, &onclose_cb);
}

bool writer_isclosed(struct writer const *writer) { return writer->closed; }

static void write_cb(struct uv_write_s *write, int status)
{
    struct request *request = write->data;
    free((void *)request->msg);
    free(request);
    if (status) eio("%s", uv_strerror(status));
}

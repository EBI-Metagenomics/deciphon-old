#include "loop/writer.h"
#include "core/logging.h"
#include "uv.h"
#include <stdbool.h>
#include <stdlib.h>

struct request
{
    struct writer *writer;
    struct uv_write_s req;
    char const *msg;
};

void writer_init(struct writer *writer, struct uv_loop_s *loop,
                 writer_onclose_fn_t *onclose_cb)
{
    writer->loop = loop;
    writer->onclose_cb = onclose_cb;
}

void writer_open(struct writer *writer, uv_file fd)
{
    if (uv_pipe_init(writer->loop, &writer->pipe, 0)) fatal("uv_pipe_init");

    ((struct uv_handle_s *)(&writer->pipe))->data = writer;
    ((struct uv_stream_s *)(&writer->pipe))->data = writer;

    if (uv_pipe_open(&writer->pipe, fd)) fatal("uv_pipe_open");
}

static void *memdup(const void *mem, size_t size)
{
    void *out = malloc(size);

    if (out != NULL) memcpy(out, mem, size);

    return out;
}

static void write_cb(struct uv_write_s *write, int status);

void writer_put(struct writer *writer, char const *msg)
{
    /* TODO: fix it */
    puts(msg);
    fflush(stdout);
    return;
    unsigned size = (unsigned)strlen(msg);
    struct request *request = malloc(sizeof(struct request));
    request->writer = writer;
    request->msg = memdup(msg, size);
    request->req.data = request;

    struct uv_stream_s *stream = (struct uv_stream_s *)&writer->pipe;
    uv_buf_t bufs[2] = {uv_buf_init((char *)request->msg, size),
                        uv_buf_init((char *)"\n", 1)};

    uv_write(&request->req, stream, bufs, 2, &write_cb);
}

void onclose_cb(struct uv_handle_s *handle)
{
    struct writer *writer = handle->data;
    (*writer->onclose_cb)();
}

void writer_close(struct writer *writer)
{
    uv_close((struct uv_handle_s *)&writer->pipe, &onclose_cb);
}

static void write_cb(struct uv_write_s *write, int status)
{
    struct request *request = write->data;
    free((void *)request->msg);
    free(request);
    if (status) eio(uv_strerror(status));
}

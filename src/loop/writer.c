#include "deciphon/loop/writer.h"
#include "deciphon/core/logging.h"
#include "deciphon/loop/looper.h"
#include "uv.h"
#include <stdbool.h>
#include <stdlib.h>

struct writer
{
    struct looper *looper;
    struct uv_pipe_s pipe;
};

struct request
{
    struct writer *writer;
    struct uv_write_s req;
    char const *msg;
};

struct writer *writer_new(struct looper *looper, uv_file fd)
{
    struct writer *writer = malloc(sizeof *writer);
    if (!writer) return 0;

    writer->looper = looper;

    if (uv_pipe_init(writer->looper->loop, &writer->pipe, fd))
        fatal("uv_pipe_init");
    ((struct uv_handle_s *)(&writer->pipe))->data = writer;
    ((struct uv_stream_s *)(&writer->pipe))->data = writer;

    if (uv_pipe_open(&writer->pipe, fd)) fatal("uv_pipe_open");
    return writer;
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

    uv_write(&request->req, stream, bufs, 2, write_cb);
}

void writer_del(struct writer *writer)
{
    uv_close((struct uv_handle_s *)&writer->pipe, 0);
    free(writer);
}

static void write_cb(struct uv_write_s *write, int status)
{
    struct request *request = write->data;
    free((void *)request->msg);
    free(request);
    if (status) eio(uv_strerror(status));
}

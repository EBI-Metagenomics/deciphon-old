#include "loop/writer.h"
#include "core/logy.h"
#include "core/xmem.h"
#include "loop/writer_req.h"
#include "loop/writer_req_pool.h"
#include "uv.h"
#include <string.h>

void writer_init(struct writer *writer, struct uv_pipe_s *pipe,
                 on_error2_fn_t *on_error)
{
    writer->pipe = pipe;
    writer->pipe->data = writer;
    writer->on_error = on_error;
    writer->closed = false;
}

static void write_fwd(struct uv_write_s *write, int status);

void writer_try_put(struct writer *writer, char const *string)
{
    if (writer->closed) return;

    uv_buf_t buf = uv_buf_init((char *)string, strlen(string));
    struct uv_stream_s *stream = (struct uv_stream_s *)writer->pipe;
    uv_try_write(stream, &buf, 1);
}

void writer_put(struct writer *writer, char const *string)
{
    if (writer->closed) return;

    struct writer_req *req = writer_req_pool_pop(writer);
    writer_req_set_string(req, string);
    static char newline[] = "\n";
    uv_buf_t bufs[2] = {uv_buf_init(req->data, req->size - 1),
                        uv_buf_init(newline, 1)};

    struct uv_stream_s *stream = (struct uv_stream_s *)writer->pipe;
    int rc = uv_write(&req->uvreq, stream, bufs, 2, &write_fwd);
    if (rc)
    {
        error("failed to write: %s", uv_strerror(rc));
        (*writer->on_error)();
    }
}

void writer_close(struct writer *writer)
{
    /* TODO: finish it */
    writer->closed = true;
}

static void write_fwd(struct uv_write_s *write, int rc)
{
    struct writer_req *req = write->data;
    struct writer *writer = req->writer;
    if (writer->closed) return;
    if (rc)
    {
        error("failed to write: %s", uv_strerror(rc));
        (*writer->on_error)();
    }
    writer_req_pool_put(req);
}

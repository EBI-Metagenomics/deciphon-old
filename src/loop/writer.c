#include "loop/writer.h"
#include "core/logy.h"
#include "core/xmem.h"
#include "loop/writer_req.h"
#include "loop/writer_req_pool.h"
#include "uv.h"

void writer_init(struct writer *writer, struct uv_pipe_s *pipe,
                 on_error_fn_t *onerror, void *arg)
{
    writer->pipe = pipe;
    writer->pipe->data = writer;

    writer->cb.onerror = onerror;
    writer->cb.arg = arg;
}

static void write_fwd(struct uv_write_s *write, int status);

void writer_try_put(struct writer *writer, char const *string)
{
    uv_buf_t buf = uv_buf_init((char *)string, strlen(string));

    struct uv_stream_s *stream = (struct uv_stream_s *)writer->pipe;
    uv_try_write(stream, &buf, 1);
}

void writer_put(struct writer *writer, char const *string)
{
    struct writer_req *req = writer_req_pool_pop(writer);
    writer_req_set_string(req, string);
    uv_buf_t buf = uv_buf_init(req->data, req->size);

    struct uv_stream_s *stream = (struct uv_stream_s *)writer->pipe;
    int rc = uv_write(&req->uvreq, stream, &buf, 1, &write_fwd);
    if (rc)
    {
        error("failed to write: %s", uv_strerror(rc));
        (*writer->cb.onerror)(writer->cb.arg);
    }
}

static void write_fwd(struct uv_write_s *write, int rc)
{
    struct writer_req *req = write->data;
    struct writer *writer = req->writer;
    if (rc)
    {
        error("failed to write: %s", uv_strerror(rc));
        (*writer->cb.onerror)(writer->cb.arg);
    }
    writer_req_pool_put(req);
}

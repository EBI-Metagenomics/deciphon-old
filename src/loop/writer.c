#include "loop/writer.h"
#include "core/logy.h"
#include "loop/wreq.h"
#include "uv.h"

void writer_init(struct writer *writer, struct uv_pipe_s *pipe,
                 on_error2_fn_t *on_error)
{
    writer->pipe = pipe;
    writer->pipe->data = writer;
    writer->on_error = on_error;
    writer->closed = 0;
}

static void write_fwd(struct uv_write_s *write, int status);

void writer_put(struct writer *writer, char const *string)
{
    if (writer->closed) return;

    struct wreq *req = wreq_pop(writer);
    wreq_setstr(req, string);
    static char newline[] = "\n";
    uv_buf_t bufs[2] = {uv_buf_init(wreq_data(req), wreq_size(req) - 1),
                        uv_buf_init(newline, 1)};

    struct uv_stream_s *stream = (struct uv_stream_s *)writer->pipe;
    int rc = uv_write(wreq_uvreq(req), stream, bufs, 2, &write_fwd);
    if (rc)
    {
        error("failed to write: %s", uv_strerror(rc));
        wreq_put(req);
        (*writer->on_error)();
    }
}

void writer_close(struct writer *writer)
{
    if (writer->closed) return;
    writer->closed = 1;
}

static void write_fwd(struct uv_write_s *write, int rc)
{
    struct wreq *req = write->data;
    struct writer *writer = wreq_writer(req);
    if (writer->closed) return;
    if (rc)
    {
        error("failed to write: %s", uv_strerror(rc));
        (*writer->on_error)();
    }
    wreq_put(req);
}

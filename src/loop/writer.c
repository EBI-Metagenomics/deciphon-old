#include "loop/writer.h"
#include "logy.h"
#include "loop/wreq.h"
#include <assert.h>
#include <uv.h>

void writer_init(struct writer *writer, struct uv_pipe_s *pipe,
                 on_error2_fn_t *on_error)
{
    writer->pipe = pipe;
    assert(!writer->pipe->data);
    writer->pipe->data = writer;
    writer->on_error = on_error;
    writer->open = 0;
}

static void write_fwd(struct uv_write_s *write, int status);

void writer_open(struct writer *writer)
{
    if (writer->open) return;
    writer->open = 1;
}

void writer_put(struct writer *writer, char const *string)
{
    if (!writer->open) return;

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
        (*writer->on_error)();
        wreq_put(req);
    }
}

void writer_close(struct writer *writer)
{
    if (!writer->open) return;
    writer->open = 0;
}

static void write_fwd(struct uv_write_s *write, int rc)
{
    struct wreq *req = write->data;
    struct writer *writer = wreq_writer(req);
    if (!writer->open) return;
    if (rc)
    {
        error("failed to write: %s", uv_strerror(rc));
        (*writer->on_error)();
    }
    wreq_put(req);
}

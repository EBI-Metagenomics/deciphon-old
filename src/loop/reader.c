#include "loop/reader.h"
#include "core/logy.h"
#include "uv.h"
#include <assert.h>
#include <string.h>

void reader_init(struct reader *rdr, struct uv_pipe_s *pipe,
                 on_eof_fn_t *on_eof, on_error_fn_t *on_error,
                 on_read_fn_t *on_read, void *arg)
{
    rdr->pipe = pipe;
    rdr->pipe->data = rdr;

    rdr->no_start = false;
    rdr->no_stop = true;

    rdr->cb.on_eof = on_eof;
    rdr->cb.on_error = on_error;
    rdr->cb.on_read = on_read;
    rdr->cb.arg = arg;

    rdr->pos = rdr->buff;
    rdr->end = rdr->pos;
}

static void alloc_buff(uv_handle_t *handle, size_t suggested_size,
                       uv_buf_t *buf);
static void read_pipe(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);

void reader_start(struct reader *rdr)
{
    if (rdr->no_start) return;
    rdr->no_start = true;
    rdr->no_stop = false;

    rdr->pos = rdr->buff;
    rdr->end = rdr->pos;
    int rc = uv_read_start((uv_stream_t *)rdr->pipe, &alloc_buff, &read_pipe);
    if (rc) fatal("uv_read_start: %s", uv_strerror(rc));
}

void reader_stop(struct reader *rdr)
{
    if (rdr->no_stop) return;
    rdr->no_stop = true;
    rdr->no_start = false;

    int rc = uv_read_stop((uv_stream_t *)rdr->pipe);
    if (rc) fatal("uv_read_stop: %s", uv_strerror(rc));
}

static void process_error(struct reader *rdr)
{
    reader_stop(rdr);
    (*rdr->cb.on_error)(rdr->cb.arg);
}

static void process_newlines(struct reader *rdr)
{
    reader_stop(rdr);
    rdr->end = rdr->pos;
    rdr->pos = rdr->buff;

    char *z = 0;

    goto enter;
    while (z)
    {
        *z = 0;
        (*rdr->cb.on_read)(rdr->pos, rdr->cb.arg);
        rdr->pos = z + 1;
    enter:
        z = memchr(rdr->pos, '\n', (unsigned)(rdr->end - rdr->pos));
    }
    if (rdr->pos < rdr->end)
    {
        size_t n = rdr->end - rdr->pos;
        memmove(rdr->buff, rdr->pos, n);
        rdr->pos = rdr->buff + n;
    }
    reader_start(rdr);
}

static void read_pipe(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    struct reader *rdr = stream->data;
    if (nread < 0)
    {
        if (nread == UV_EOF)
            (*rdr->cb.on_eof)(rdr->cb.arg);
        else
        {
            error("failed to read: %s", uv_strerror(nread));
            process_error(rdr);
        }
    }
    else if (nread > 0)
    {
        assert(nread <= READER_LINE_SIZE);
        unsigned count = (unsigned)nread;
        size_t avail = READER_BUFF_SIZE - (rdr->pos - rdr->buff);
        if (avail < count)
        {
            error("not-enough-memory to read");
            process_error(rdr);
            return;
        }

        memcpy(rdr->pos, buf->base, count);
        rdr->pos += count;
        if (memchr(buf->base, '\n', count))
        {
            process_newlines(rdr);
        }
    }
}

static void alloc_buff(uv_handle_t *handle, size_t suggested_size,
                       uv_buf_t *buf)
{
    (void)suggested_size;
    struct reader *rdr = handle->data;
    *buf = uv_buf_init(rdr->mem, READER_LINE_SIZE);
}

void reader_close(struct reader *rdr)
{
    reader_stop(rdr);
    rdr->no_start = true;
    rdr->no_stop = true;
}

#include "loop/reader.h"
#include "core/logy.h"
#include "uv.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void reader_init(struct reader *reader, struct uv_pipe_s *pipe,
                 on_eof_fn_t *oneof, on_error_fn_t *onerror,
                 on_read_fn_t *onread, void *arg)
{
    reader->pipe = pipe;
    reader->pipe->data = reader;

    reader->cb.oneof = oneof;
    reader->cb.onerror = onerror;
    reader->cb.onread = onread;
    reader->cb.arg = arg;

    reader->pos = reader->buff;
    reader->end = reader->pos;
}

static void start_reading(struct reader *);
static void stop_reading(struct reader *);

void reader_start(struct reader *reader) { start_reading(reader); }

void reader_stop(struct reader *reader) { stop_reading(reader); }

static void process_error(struct reader *reader)
{
    stop_reading(reader);
    (*reader->cb.onerror)(reader->cb.arg);
}

static void process_newlines(struct reader *reader)
{
    stop_reading(reader);
    reader->end = reader->pos;
    reader->pos = reader->buff;

    char *z = 0;

    goto enter;
    while (z)
    {
        *z = 0;
        (*reader->cb.onread)(reader->pos, reader->cb.arg);
        reader->pos = z + 1;
    enter:
        z = memchr(reader->pos, '\n', (unsigned)(reader->end - reader->pos));
    }
    if (reader->pos < reader->end)
    {
        size_t n = reader->end - reader->pos;
        memmove(reader->buff, reader->pos, n);
        reader->pos = reader->buff + n;
    }
    start_reading(reader);
}

static void read_pipe(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    struct reader *reader = stream->data;
    if (nread < 0)
    {
        if (nread == UV_EOF)
            (*reader->cb.oneof)(reader->cb.arg);
        else
            process_error(reader);
    }
    else if (nread > 0)
    {
        assert(nread <= READER_LINE_SIZE);
        unsigned count = (unsigned)nread;
        size_t avail = READER_BUFF_SIZE - (reader->pos - reader->buff);
        if (avail < count)
        {
            process_error(reader);
            return;
        }

        memcpy(reader->pos, buf->base, count);
        reader->pos += count;
        if (memchr(buf->base, '\n', count))
        {
            process_newlines(reader);
        }
    }
}

static void alloc_buff(uv_handle_t *handle, size_t suggested_size,
                       uv_buf_t *buf)
{
    (void)suggested_size;
    struct reader *reader = handle->data;
    *buf = uv_buf_init(reader->mem, READER_LINE_SIZE);
}

static void start_reading(struct reader *rdr)
{
    rdr->pos = rdr->buff;
    rdr->end = rdr->pos;
    int rc = uv_read_start((uv_stream_t *)rdr->pipe, &alloc_buff, &read_pipe);
    if (rc) fatal("uv_read_start: %s", uv_strerror(rc));
}

static void stop_reading(struct reader *reader)
{
    int rc = uv_read_stop((uv_stream_t *)reader->pipe);
    if (rc) fatal("uv_read_stop: %s", uv_strerror(rc));
}

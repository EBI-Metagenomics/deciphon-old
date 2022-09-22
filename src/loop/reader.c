#include "loop/reader.h"
#include "core/logging.h"
#include "uv.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void alloc_buff(uv_handle_t *handle, size_t suggested_size,
                       uv_buf_t *buf);
static void onclose_cb(struct uv_handle_s *handle);
static void process_error(struct reader *);
static void process_newlines(struct reader *);
static void read_pipe(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
static void start_reading(struct reader *);
static void stop_reading(struct reader *);

void reader_init(struct reader *reader, struct uv_loop_s *loop,
                 reader_onerror_fn_t *onerror_cb, reader_onread_fn_t *onread_cb,
                 reader_onclose_fn_t *onclose_cb, void *arg)
{
    reader->loop = loop;
    reader->noclose = true;
    reader->onerror_cb = onerror_cb;
    reader->onread_cb = onread_cb;
    reader->onclose_cb = onclose_cb;
    reader->arg = arg;
    reader->pos = reader->buff;
    reader->end = reader->pos;
}

void reader_open(struct reader *reader, uv_file fd)
{
    if (uv_pipe_init(reader->loop, &reader->pipe, 0)) fatal("uv_pipe_init");
    ((struct uv_handle_s *)(&reader->pipe))->data = reader;
    ((struct uv_stream_s *)(&reader->pipe))->data = reader;

    if (uv_pipe_open(&reader->pipe, fd)) fatal("uv_pipe_open");
    start_reading(reader);
    reader->noclose = false;
}

void reader_close(struct reader *reader)
{
    stop_reading(reader);
    if (reader->noclose) return;
    uv_close((struct uv_handle_s *)&reader->pipe, &onclose_cb);
    reader->noclose = true;
}

static void alloc_buff(uv_handle_t *handle, size_t suggested_size,
                       uv_buf_t *buf)
{
    (void)suggested_size;
    struct reader *reader = handle->data;
    *buf = uv_buf_init(reader->mem, READER_LINE_SIZE);
}

static void onclose_cb(struct uv_handle_s *handle)
{
    struct reader *reader = handle->data;
    (*reader->onclose_cb)(reader->arg);
}

static void process_error(struct reader *reader)
{
    stop_reading(reader);
    (*reader->onerror_cb)(reader->arg);
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
        (*reader->onread_cb)(reader->pos, reader->arg);
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
            reader_close(reader);
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

static void start_reading(struct reader *reader)
{
    reader->pos = reader->buff;
    reader->end = reader->pos;
    if (uv_read_start((uv_stream_t *)&reader->pipe, alloc_buff, read_pipe))
        fatal("uv_read_start");
}

static void stop_reading(struct reader *reader)
{
    if (uv_read_stop((uv_stream_t *)&reader->pipe)) fatal("uv_read_stop");
}
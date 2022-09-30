#include "loop/reader.h"
#include "core/logy.h"
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

void reader_init(struct reader *reader, struct uv_loop_s *loop, int ipc,
                 reader_oneof_fn_t *oneof_cb, reader_onerror_fn_t *onerror_cb,
                 reader_onread_fn_t *onread_cb, reader_onclose_fn_t *onclose_cb,
                 void *arg)
{
    reader->loop = loop;
    reader->oneof_cb = oneof_cb;
    reader->onerror_cb = onerror_cb;
    reader->onread_cb = onread_cb;
    reader->onclose_cb = onclose_cb;
    reader->arg = arg;
    reader->closed = false;
    reader->nostart_reading = false;
    reader->pos = reader->buff;
    reader->end = reader->pos;
    uv_pipe_init(reader->loop, &reader->pipe, ipc);
    ((struct uv_handle_s *)(&reader->pipe))->data = reader;
    ((struct uv_stream_s *)(&reader->pipe))->data = reader;
}

void reader_fopen(struct reader *reader, int fd)
{
    int rc = uv_pipe_open(&reader->pipe, fd);
    if (rc) fatal("%s", uv_strerror(rc));
    start_reading(reader);
    reader->closed = false;
}

void reader_start(struct reader *reader) { start_reading(reader); }

struct uv_pipe_s *reader_pipe(struct reader *reader) { return &reader->pipe; }

void reader_close(struct reader *reader)
{
    assert(!reader->closed);
    reader->nostart_reading = true;
    stop_reading(reader);
    uv_close((struct uv_handle_s *)&reader->pipe, &onclose_cb);
}

bool reader_isclosed(struct reader const *reader) { return reader->closed; }

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
    reader->closed = true;
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
            (*reader->oneof_cb)(reader->arg);
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
    if (reader->nostart_reading) return;
    reader->pos = reader->buff;
    reader->end = reader->pos;
    if (uv_read_start((uv_stream_t *)&reader->pipe, alloc_buff, read_pipe))
        fatal("uv_read_start");
}

static void stop_reading(struct reader *reader)
{
    if (uv_read_stop((uv_stream_t *)&reader->pipe)) fatal("uv_read_stop");
}

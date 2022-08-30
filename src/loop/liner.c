#include "deciphon/loop/liner.h"
#include "deciphon/core/logging.h"
#include "deciphon/loop/looper.h"
#include "uv.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void alloc_buff(uv_handle_t *handle, size_t suggested_size,
                       uv_buf_t *buf);
static void read_pipe(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
static void process_error(struct liner *);
static void process_newlines(struct liner *);
static void start_reading(struct liner *);
static void stop_reading(struct liner *);

void liner_init(struct liner *liner, struct looper *looper,
                liner_ioerror_fn_t *ioerror_cb, liner_newline_fn_t *newline_cb)
{
    liner->looper = looper;
    liner->noclose = true;
    liner->ioerror_cb = ioerror_cb;
    liner->newline_cb = newline_cb;
    liner->pos = liner->buff;
    liner->end = liner->pos;
}

void liner_open(struct liner *liner, uv_file fd)
{
    if (uv_pipe_init(liner->looper->loop, &liner->pipe, fd))
        fatal("uv_pipe_init");
    ((struct uv_handle_s *)(&liner->pipe))->data = liner;
    ((struct uv_stream_s *)(&liner->pipe))->data = liner;

    if (uv_pipe_open(&liner->pipe, fd)) fatal("uv_pipe_open");
    start_reading(liner);
    liner->noclose = false;
}

void liner_close(struct liner *liner)
{
    stop_reading(liner);
    if (liner->noclose) return;
    uv_close((struct uv_handle_s *)&liner->pipe, 0);
    liner->noclose = true;
}

static void alloc_buff(uv_handle_t *handle, size_t suggested_size,
                       uv_buf_t *buf)
{
    (void)suggested_size;
    struct liner *liner = handle->data;
    *buf = uv_buf_init(liner->mem, LINER_LINE_SIZE);
}

static void read_pipe(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    struct liner *liner = stream->data;
    if (nread < 0)
    {
        if (nread == UV_EOF)
            liner_close(liner);
        else
            process_error(liner);
    }
    else if (nread > 0)
    {
        assert(nread <= LINER_LINE_SIZE);
        unsigned count = (unsigned)nread;
        size_t avail = LINER_BUFF_SIZE - (liner->pos - liner->buff);
        if (avail < count)
        {
            process_error(liner);
            return;
        }

        memcpy(liner->pos, buf->base, count);
        liner->pos += count;
        if (memchr(buf->base, '\n', count))
        {
            process_newlines(liner);
        }
    }
}

void process_error(struct liner *liner)
{
    stop_reading(liner);
    (*liner->ioerror_cb)();
}

static void process_newlines(struct liner *liner)
{
    stop_reading(liner);
    liner->end = liner->pos;
    liner->pos = liner->buff;

    char *z = 0;

    goto enter;
    while (z)
    {
        *z = 0;
        (*liner->newline_cb)(liner->pos);
        liner->pos = z + 1;
    enter:
        z = memchr(liner->pos, '\n', (unsigned)(liner->end - liner->pos));
    }
    if (liner->pos < liner->end)
    {
        size_t n = liner->end - liner->pos;
        memmove(liner->buff, liner->pos, n);
        liner->pos = liner->buff + n;
    }
    start_reading(liner);
}

static void start_reading(struct liner *liner)
{
    liner->pos = liner->buff;
    liner->end = liner->pos;
    if (uv_read_start((uv_stream_t *)&liner->pipe, alloc_buff, read_pipe))
        fatal("uv_read_start");
}

static void stop_reading(struct liner *liner)
{
    if (uv_read_stop((uv_stream_t *)&liner->pipe)) fatal("uv_read_stop");
}

#include "loop/reader.h"
#include "core/die.h"
#include "core/logy.h"
#include "core/pp.h"
#include "uv.h"
#include <assert.h>
#include <ctype.h>
#include <string.h>

static bool only_spaces(char const *string)
{
    char const *p = string;
    unsigned count = 0;
    while (*p)
    {
        count += !isspace(*p);
        ++p;
    }
    return count == 0;
}

static void on_read_fwd(char *line, void *arg)
{
    if (only_spaces(line)) return;
    struct reader *rdr = arg;
    if (rdr->on_read) (*rdr->on_read)(line);
}

void reader_init(struct reader *rdr, struct uv_pipe_s *pipe,
                 on_read2_fn_t *on_read, on_eof2_fn_t *on_eof,
                 on_error2_fn_t *on_error)
{
    rdr->pipe = pipe;
    assert(!rdr->pipe->data);
    rdr->pipe->data = rdr;

    rdr->open = 0;

    rdr->on_eof = on_eof;
    rdr->on_error = on_error;
    rdr->on_read = on_read;

    rdr->pos = rdr->buf;
    rdr->end = rdr->pos;
}

static void alloc_buff(uv_handle_t *handle, size_t suggested_size,
                       uv_buf_t *buf);
static void read_pipe(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);

void reader_open(struct reader *rdr)
{
    if (rdr->open) return;
    rdr->open = 1;

    rdr->pos = rdr->buf;
    rdr->end = rdr->pos;
    if (uv_read_start((uv_stream_t *)rdr->pipe, &alloc_buff, &read_pipe)) die();
}

void reader_close(struct reader *rdr)
{
    if (!rdr->open) return;
    rdr->open = 0;

    if (uv_read_stop((uv_stream_t *)rdr->pipe)) die();
}

static void process_error(struct reader *rdr)
{
    reader_close(rdr);
    if (rdr->on_error) (*rdr->on_error)();
}

static void process_newlines(struct reader *rdr)
{
    reader_close(rdr);
    rdr->end = rdr->pos;
    rdr->pos = rdr->buf;

    char *z = 0;

    goto enter;
    while (z)
    {
        *z = 0;
        on_read_fwd(rdr->pos, rdr);
        rdr->pos = z + 1;
    enter:
        z = memchr(rdr->pos, '\n', (unsigned)(rdr->end - rdr->pos));
    }
    if (rdr->pos < rdr->end)
    {
        size_t n = rdr->end - rdr->pos;
        memmove(rdr->buf, rdr->pos, n);
        rdr->pos = rdr->buf + n;
    }
    reader_open(rdr);
}

static void read_pipe(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    struct reader *rdr = stream->data;
    if (nread < 0)
    {
        if (nread == UV_EOF && rdr->on_eof)
            (*rdr->on_eof)();
        else
        {
            error("failed to read: %s", uv_strerror(nread));
            process_error(rdr);
        }
    }
    else if (nread > 0)
    {
        assert(nread <= (ssize_t)sizeof_field(struct reader, mem));
        unsigned count = (unsigned)nread;
        size_t avail = sizeof_field(struct reader, buf) - (rdr->pos - rdr->buf);
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
    *buf = uv_buf_init(rdr->mem, sizeof_field(struct reader, mem));
}

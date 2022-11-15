#include "loop/input.h"
#include "core/global.h"
#include <ctype.h>

static void fwd_eof(void *arg);
static void fwd_error(void *arg);
static void fwd_read(char *line, void *arg);

void input_init(struct input *i, int fd, on_read2_fn_t *on_read,
                on_eof2_fn_t *on_eof, on_error2_fn_t *on_error,
                on_exit2_fn_t *on_exit, void *arg)
{
    stdpipe_init(&i->pipe, on_exit, arg);
    i->fd = fd;
    i->on_read = on_read;
    i->on_eof = on_eof;
    i->on_error = on_error;
    reader_init(&i->reader, &i->pipe.uvpipe, fwd_eof, fwd_error, fwd_read, i);
}

void input_start(struct input *i)
{
    if (i->fd >= 0) stdpipe_open(&i->pipe, i->fd);
    reader_open(&i->reader);
}

void input_stop(struct input *i)
{
    reader_close(&i->reader);
    stdpipe_close(&i->pipe);
}

void input_close(struct input *i)
{
    reader_close(&i->reader);
    stdpipe_close(&i->pipe);
}

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

static void fwd_eof(void *arg)
{
    struct input *i = arg;
    reader_close(&i->reader);
    if (i->on_eof) (*i->on_eof)();
}

static void fwd_error(void *arg)
{
    struct input *i = arg;
    reader_close(&i->reader);
    if (i->on_error) (*i->on_error)();
}

static void fwd_read(char *line, void *arg)
{
    if (only_spaces(line)) return;
    struct input *i = arg;
    if (i->on_read) (*i->on_read)(line);
}

#include "loop/input.h"
#include "core/global.h"
#include "core/logy.h"
#include <ctype.h>

void input_init(struct input *i, int fd)
{
    stdpipe_init(&i->pipe, global_loop());
    if (fd >= 0) stdpipe_open(&i->pipe, fd);
    i->cb.on_eof = NULL;
    i->cb.on_error = NULL;
    i->cb.on_read = NULL;
}

void input_setup(struct input *i, on_eof2_fn_t *on_eof,
                 on_error2_fn_t *on_error, on_read2_fn_t *on_read)
{
    i->cb.on_eof = on_eof;
    i->cb.on_error = on_error;
    i->cb.on_read = on_read;
}

void input_forward(struct input *i, char *line)
{
    if (i->cb.on_read) (*i->cb.on_read)(line);
}

static void fwd_eof(void *arg)
{
    struct input *i = arg;
    reader_stop(&i->reader);
    if (i->cb.on_eof) (*i->cb.on_eof)();
}

static void fwd_error(void *arg)
{
    struct input *i = arg;
    reader_stop(&i->reader);
    if (i->cb.on_error) (*i->cb.on_error)();
}

static bool only_spaces(char const *string);

static void fwd_read(char *line, void *arg)
{
    if (only_spaces(line)) return;
    struct input *i = arg;
    if (i->cb.on_read) (*i->cb.on_read)(line);
}

void input_start(struct input *i)
{
    reader_init(&i->reader, &i->pipe.uvpipe, fwd_eof, fwd_error, fwd_read, i);
    reader_start(&i->reader);
}

void input_stop(struct input *i) { reader_stop(&i->reader); }

void input_close(struct input *i) { stdpipe_close(&i->pipe); }

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

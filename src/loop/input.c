#include "loop/input.h"
#include "core/global.h"
#include <ctype.h>

void input_init(struct input *i, int fd, on_exit2_fn_t *on_exit, void *arg)
{
    stdpipe_init(&i->pipe, global_loop(), on_exit, arg);
    if (fd >= 0) stdpipe_open(&i->pipe, fd);
    i->on_eof = NULL;
    i->on_error = NULL;
    i->on_read = NULL;
}

void input_setup(struct input *i, on_eof2_fn_t *on_eof,
                 on_error2_fn_t *on_error, on_read2_fn_t *on_read)
{
    i->on_eof = on_eof;
    i->on_error = on_error;
    i->on_read = on_read;
}

static void fwd_eof(void *arg)
{
    struct input *i = arg;
    reader_stop(&i->reader);
    if (i->on_eof) (*i->on_eof)();
}

static void fwd_error(void *arg)
{
    struct input *i = arg;
    reader_stop(&i->reader);
    if (i->on_error) (*i->on_error)();
}

static bool only_spaces(char const *string);

static void fwd_read(char *line, void *arg)
{
    if (only_spaces(line)) return;
    struct input *i = arg;
    if (i->on_read) (*i->on_read)(line);
}

void input_start(struct input *i)
{
    reader_init(&i->reader, &i->pipe.uvpipe, fwd_eof, fwd_error, fwd_read, i);
    reader_start(&i->reader);
}

void input_stop(struct input *i) { reader_stop(&i->reader); }

void input_cleanup(struct input *i)
{
    reader_cleanup(&i->reader);
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

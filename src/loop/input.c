#include "input.h"
#include "global.h"
#include "logy.h"

void input_init(struct input *i, int fd)
{
    stdpipe_init(&i->pipe, global_loop());
    if (fd >= 0) stdpipe_open(&i->pipe, fd);
    i->cb.on_eof = NULL;
    i->cb.on_error = NULL;
    i->cb.on_read = NULL;
    i->cb.arg = NULL;
}

struct input_cb *input_cb(struct input *i) { return &i->cb; }

static void fwd_eof(void *arg)
{
    struct input *i = arg;
    reader_stop(&i->reader);
    if (i->cb.on_eof) (*i->cb.on_eof)(i->cb.arg);
}

static void fwd_error(void *arg)
{
    struct input *i = arg;
    reader_stop(&i->reader);
    if (i->cb.on_error) (*i->cb.on_error)(i->cb.arg);
}

static void fwd_read(char *line, void *arg)
{
    struct input *i = arg;
    if (i->cb.on_read) (*i->cb.on_read)(line, i->cb.arg);
}

void input_start(struct input *i)
{
    reader_init(&i->reader, &i->pipe.uvpipe, fwd_eof, fwd_error, fwd_read, i);
    reader_start(&i->reader);
}

void input_stop(struct input *i) { reader_stop(&i->reader); }

void input_close(struct input *i) { stdpipe_close(&i->pipe); }
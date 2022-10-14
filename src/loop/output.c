#include "loop/output.h"
#include "core/global.h"
#include "core/logy.h"

void output_init(struct output *o, int fd)
{
    stdpipe_init(&o->pipe, global_loop());
    if (fd >= 0) stdpipe_open(&o->pipe, fd);
    o->cb.on_error = NULL;
}

void output_setup(struct output *o, on_error2_fn_t *on_error)
{
    o->cb.on_error = on_error;
}

void output_start(struct output *o)
{
    writer_init(&o->writer, &o->pipe.uvpipe, o->cb.on_error);
}

void output_put(struct output *o, char const *string)
{
    if (string) writer_put(&o->writer, string);
}

void output_close(struct output *o) { stdpipe_close(&o->pipe); }

#include "loop/output.h"
#include "core/global.h"
#include <assert.h>

void output_init(struct output *o, int fd, on_error2_fn_t *on_error,
                 on_exit2_fn_t *on_exit, void *arg)
{
    stdpipe_init(&o->pipe, global_loop(), on_exit, arg);
    if (fd >= 0) stdpipe_open(&o->pipe, fd);
    o->on_error = on_error;
    writer_init(&o->writer, &o->pipe.uvpipe, o->on_error);
}

void output_put(struct output *o, char const *string)
{
    if (string) writer_put(&o->writer, string);
}

void output_close(struct output *o)
{
    writer_close(&o->writer);
    stdpipe_close(&o->pipe);
}

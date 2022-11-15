#ifndef LOOP_WRITER_H
#define LOOP_WRITER_H

#include "loop/callbacks.h"
#include "uv.h"

struct uv_pipe_s;

struct writer
{
    struct uv_pipe_s *pipe;
    on_error2_fn_t *on_error;
    int closed;
};

void writer_init(struct writer *, struct uv_pipe_s *, on_error2_fn_t *);
void writer_put(struct writer *, char const *string);
void writer_close(struct writer *);

#endif

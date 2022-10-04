#ifndef PIPE_WRITER_H
#define PIPE_WRITER_H

#include "callbacks.h"
#include "uv.h"
#include <stdbool.h>

struct uv_pipe_s;

struct writer
{
    struct uv_pipe_s *pipe;

    struct
    {
        on_error_fn_t *onerror;
        void *arg;
    } cb;
};

void writer_init(struct writer *, struct uv_pipe_s *, on_error_fn_t *,
                 void *arg);
void writer_try_put(struct writer *, char const *string);
void writer_put(struct writer *, char const *string);

#endif

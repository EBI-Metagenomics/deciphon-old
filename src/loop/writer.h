#ifndef LOOP_WRITER_H
#define LOOP_WRITER_H

#include "uv.h"
#include <stdbool.h>

typedef void writer_onclose_fn_t(void);

struct writer
{
    struct uv_loop_s *loop;
    struct uv_pipe_s pipe;
    writer_onclose_fn_t *onclose_cb;
};

void writer_init(struct writer *, struct uv_loop_s *loop,
                 writer_onclose_fn_t *);
void writer_open(struct writer *, uv_file fd);
void writer_put(struct writer *, char const *msg);
void writer_close(struct writer *);

#endif

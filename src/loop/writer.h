#ifndef LOOP_WRITER_H
#define LOOP_WRITER_H

#include "uv.h"
#include <stdbool.h>

typedef void writer_onerror_fn_t(void *arg);
typedef void writer_onclose_fn_t(void *arg);

struct writer
{
    struct uv_loop_s *loop;
    struct uv_pipe_s pipe;
    writer_onclose_fn_t *onerror_cb;
    writer_onclose_fn_t *onclose_cb;
    void *arg;
};

void writer_init(struct writer *, struct uv_loop_s *loop, writer_onerror_fn_t *,
                 writer_onclose_fn_t *, void *arg);
void writer_fopen(struct writer *, int fd);
void writer_put(struct writer *, char const *msg);
struct uv_pipe_s *writer_pipe(struct writer *);
void writer_close(struct writer *);

#endif

#ifndef LOOP_READER_H
#define LOOP_READER_H

#include "core/pp.h"
#include "loop/callbacks.h"

struct uv_pipe_s;

struct reader
{
    struct uv_pipe_s *pipe;
    int open;

    struct
    {
        on_eof_fn_t *on_eof;
        on_error_fn_t *on_error;
        on_read_fn_t *on_read;
        void *arg;
    } cb;

    char *pos;
    char *end;
    char buf[1024];
    char mem[2048];
};

void reader_init(struct reader *, struct uv_pipe_s *, on_eof_fn_t *,
                 on_error_fn_t *, on_read_fn_t *, void *);
void reader_open(struct reader *);
void reader_close(struct reader *);

#endif

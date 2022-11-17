#ifndef LOOP_READER_H
#define LOOP_READER_H

#include "loop/callbacks.h"

struct uv_pipe_s;

struct reader
{
    struct uv_pipe_s *pipe;
    int open;

    on_eof2_fn_t *on_eof;
    on_error2_fn_t *on_error;
    on_read2_fn_t *on_read;
    void *userdata;

    char *pos;
    char *end;
    char buf[1024];
    char mem[2048];
};

void reader_init(struct reader *, struct uv_pipe_s *, on_read2_fn_t *,
                 on_eof2_fn_t *, on_error2_fn_t *);
void reader_open(struct reader *);
void reader_close(struct reader *);

#endif

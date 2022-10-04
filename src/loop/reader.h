#ifndef PIPE_READER_H
#define PIPE_READER_H

#include "callbacks.h"
#include <stdbool.h>

struct uv_pipe_s;

enum
{
    READER_LINE_SIZE = 1024,
    READER_BUFF_SIZE = 2048
};

struct reader
{
    struct uv_pipe_s *pipe;

    struct
    {
        on_eof_fn_t *oneof;
        on_error_fn_t *onerror;
        on_read_fn_t *onread;
        void *arg;
    } cb;

    char *pos;
    char *end;
    char buff[READER_BUFF_SIZE];
    char mem[READER_LINE_SIZE];
};

void reader_init(struct reader *, struct uv_pipe_s *, on_eof_fn_t *,
                 on_error_fn_t *, on_read_fn_t *, void *arg);
void reader_start(struct reader *);
void reader_stop(struct reader *);

#endif

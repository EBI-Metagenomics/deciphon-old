#ifndef LOOP_READER_H
#define LOOP_READER_H

#include "uv.h"
#include <stdbool.h>

typedef void reader_oneof_fn_t(void *arg);
typedef void reader_onerror_fn_t(void *arg);
typedef void reader_onread_fn_t(char *line, void *arg);
typedef void reader_onclose_fn_t(void *arg);

enum
{
    READER_LINE_SIZE = 1024,
    READER_BUFF_SIZE = 2048
};

struct reader
{
    struct uv_loop_s *loop;
    struct uv_pipe_s pipe;

    reader_oneof_fn_t *oneof_cb;
    reader_onerror_fn_t *onerror_cb;
    reader_onread_fn_t *onread_cb;
    reader_onclose_fn_t *onclose_cb;
    void *arg;

    char *pos;
    char *end;
    char buff[READER_BUFF_SIZE];
    char mem[READER_LINE_SIZE];
};

void reader_init(struct reader *, struct uv_loop_s *, reader_oneof_fn_t *,
                 reader_onerror_fn_t *, reader_onread_fn_t *,
                 reader_onclose_fn_t *, void *arg);
void reader_fopen(struct reader *, int fd);
void reader_close(struct reader *);

#endif

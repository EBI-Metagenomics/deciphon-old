#ifndef LOOP_READER_H
#define LOOP_READER_H

#include "uv.h"
#include <stdbool.h>

typedef void reader_onerror_fn_t(void);
typedef void reader_onread_fn_t(char *line);
typedef void reader_onclose_fn_t(void);

enum
{
    READER_LINE_SIZE = 1024,
    READER_BUFF_SIZE = 2048
};

struct reader
{
    struct uv_loop_s *loop;
    struct uv_pipe_s pipe;
    bool noclose;

    reader_onerror_fn_t *onerror_cb;
    reader_onread_fn_t *onread_cb;
    reader_onclose_fn_t *onclose_cb;

    char *pos;
    char *end;
    char buff[READER_BUFF_SIZE];
    char mem[READER_LINE_SIZE];
};

void reader_init(struct reader *, struct uv_loop_s *, reader_onerror_fn_t *,
                 reader_onread_fn_t *, reader_onclose_fn_t *);
void reader_open(struct reader *, uv_file fd);
void reader_close(struct reader *);

#endif

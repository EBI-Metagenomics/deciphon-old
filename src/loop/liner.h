#ifndef LOOP_LINER_H
#define LOOP_LINER_H

#include "uv.h"
#include <stdbool.h>

typedef void liner_ioerror_fn_t(void);
typedef void liner_newline_fn_t(char *line);
typedef void liner_onclose_fn_t(void);

enum
{
    LINER_LINE_SIZE = 1024,
    LINER_BUFF_SIZE = 2048
};

struct liner;

struct liner
{
    struct uv_loop_s *loop;
    struct uv_pipe_s pipe;
    bool noclose;

    liner_ioerror_fn_t *ioerror_cb;
    liner_newline_fn_t *newline_cb;
    liner_onclose_fn_t *onclose_cb;

    char *pos;
    char *end;
    char buff[LINER_BUFF_SIZE];
    char mem[LINER_LINE_SIZE];
};

void liner_init(struct liner *, struct uv_loop_s *, liner_ioerror_fn_t *,
                liner_newline_fn_t *, liner_onclose_fn_t *);
void liner_open(struct liner *, uv_file fd);
void liner_close(struct liner *);

#endif

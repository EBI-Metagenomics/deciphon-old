#ifndef PIPE_INPUT_H
#define PIPE_INPUT_H

#include "callbacks.h"
#include "reader.h"
#include "stdpipe.h"

struct input_cb
{
    on_eof_fn_t *on_eof;
    on_error_fn_t *on_error;
    on_read_fn_t *on_read;
    void *arg;
};

struct input
{
    struct stdpipe pipe;
    struct reader reader;
    struct input_cb cb;
};

void input_init(struct input *, int fd);
struct input_cb *input_cb(struct input *);
void input_start(struct input *);
void input_stop(struct input *);
void input_close(struct input *);

#endif

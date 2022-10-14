#ifndef LOOP_INPUT_H
#define LOOP_INPUT_H

#include "loop/callbacks.h"
#include "loop/reader.h"
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
void input_setup(struct input *, on_eof_fn_t *, on_error_fn_t *, on_read_fn_t *,
                 void *);
void input_start(struct input *);
void input_stop(struct input *);
void input_close(struct input *);

#endif

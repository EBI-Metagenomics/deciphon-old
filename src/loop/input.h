#ifndef LOOP_INPUT_H
#define LOOP_INPUT_H

#include "loop/callbacks.h"
#include "loop/reader.h"
#include "stdpipe.h"

struct input
{
    struct stdpipe pipe;
    struct reader reader;
    on_eof2_fn_t *on_eof;
    on_error2_fn_t *on_error;
    on_read2_fn_t *on_read;
};

void input_init(struct input *, int fd, on_exit2_fn_t *, void *);
void input_setup(struct input *, on_eof2_fn_t *, on_error2_fn_t *,
                 on_read2_fn_t *);
void input_start(struct input *);
void input_stop(struct input *);
void input_cleanup(struct input *);

#endif

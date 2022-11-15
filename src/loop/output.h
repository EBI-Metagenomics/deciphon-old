#ifndef LOOP_OUTPUT_H
#define LOOP_OUTPUT_H

#include "loop/callbacks.h"
#include "loop/stdpipe.h"
#include "loop/writer.h"

struct output
{
    struct stdpipe pipe;
    struct writer writer;
    int fd;
    on_error2_fn_t *on_error;
};

void output_init(struct output *, int fd, on_error2_fn_t *, on_exit2_fn_t *,
                 void *);
void output_open(struct output *);
void output_put(struct output *, char const *string);
void output_close(struct output *);

#endif

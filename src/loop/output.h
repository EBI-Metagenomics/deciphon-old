#ifndef LOOP_OUTPUT_H
#define LOOP_OUTPUT_H

#include "loop/callbacks.h"
#include "loop/stdpipe.h"
#include "writer.h"

struct output
{
    struct stdpipe pipe;
    struct writer writer;
    on_error2_fn_t *on_error;
};

void output_init(struct output *, int fd, on_exit2_fn_t *, void *);
void output_setup(struct output *, on_error2_fn_t *);
void output_start(struct output *o);
void output_put(struct output *, char const *string);
void output_cleanup(struct output *);

#endif

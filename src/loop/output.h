#ifndef LOOP_OUTPUT_H
#define LOOP_OUTPUT_H

#include "loop/callbacks.h"
#include "loop/stdpipe.h"
#include "writer.h"

struct output_cb
{
    on_error_fn_t *on_error;
    void *arg;
};

struct output
{
    struct stdpipe pipe;
    struct writer writer;
    struct output_cb cb;
};

void output_init(struct output *, int fd);
void output_setup(struct output *, on_error_fn_t *, void *);
void output_start(struct output *o);
void output_put(struct output *, char const *string);
void output_close(struct output *);

#endif

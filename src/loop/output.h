#ifndef PIPE_OUTPUT_H
#define PIPE_OUTPUT_H

#include "callbacks.h"
#include "stdpipe.h"
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
void output_start(struct output *o);
struct output_cb *output_cb(struct output *);
void output_put(struct output *, char const *string);
void output_close(struct output *);

#endif

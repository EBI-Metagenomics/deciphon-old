#ifndef LOOP_PARENT_H
#define LOOP_PARENT_H

#include "loop/callbacks.h"
#include "loop/input.h"
#include "loop/output.h"

struct parent
{
    struct input input;
    struct output output;
};

void parent_init(struct parent *, on_read2_fn_t *, on_eof2_fn_t *,
                 on_error2_fn_t *);
void parent_start(struct parent *);
void parent_send(struct parent *, char const *string);
void parent_stop(struct parent *);

#endif

#ifndef CORE_LOOP_PARENT_H
#define CORE_LOOP_PARENT_H

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
void parent_open(struct parent *);
void parent_send(struct parent *, char const *string);
void parent_close(struct parent *);

#endif

#ifndef CORE_CHILD_H
#define CORE_CHILD_H

#include "loop/callbacks.h"
#include "loop/input.h"
#include "loop/output.h"

struct child
{
    struct input input;
    struct output output;

    struct uv_process_s proc;
    struct uv_process_options_s opts;
    struct uv_stdio_container_s stdio[3];

    on_exit_fn_t *on_exit;
    bool alive;
};

void child_init(struct child *, on_read2_fn_t *, on_eof2_fn_t *,
                on_error2_fn_t *, on_exit_fn_t *);
void child_spawn(struct child *, char const *args[]);
void child_send(struct child *, char const *string);
void child_kill(struct child *);

#endif

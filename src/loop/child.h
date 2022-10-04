#ifndef CORE_CHILD_H
#define CORE_CHILD_H

#include "loop/callbacks.h"
#include "loop/input.h"
#include "loop/output.h"

struct child_cb
{
    on_exit_fn_t *on_exit;
    void *arg;
};

struct child
{
    struct input input;
    struct output output;

    struct uv_process_s proc;
    struct uv_process_options_s opts;
    struct uv_stdio_container_s stdio[3];

    struct child_cb cb;
    bool alive;
};

void child_init(struct child *child);
struct child_cb *child_cb(struct child *child);
struct input_cb *child_input_cb(struct child *child);
struct output_cb *child_output_cb(struct child *child);
void child_spawn(struct child *child, char *args[]);
void child_send(struct child *child, char const *string);
void child_stop_reading(struct child *child);
void child_kill(struct child *child);

#endif

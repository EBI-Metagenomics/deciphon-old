#ifndef LOOP_CHILD_H
#define LOOP_CHILD_H

#include "loop/callbacks.h"

struct child;

struct child *child_new(on_read2_fn_t *, on_eof2_fn_t *, on_error2_fn_t *,
                        on_exit4_fn_t *, void *);
void child_spawn(struct child *, char const *args[]);
void child_send(struct child *, char const *string);
void child_kill(struct child *);
bool child_closed(struct child const *);
void child_close(struct child *);
void child_del(struct child *);

#endif

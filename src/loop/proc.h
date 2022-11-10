#ifndef LOOP_PROC_H
#define LOOP_PROC_H

#include "loop/child.h"
#include "loop/parent.h"

enum proc_type
{
    PROC_PARENT,
    PROC_CHILD
};

struct proc
{
    enum proc_type type;
    union
    {
        struct parent parent;
        struct child child;
    };
};

void proc_init(struct proc *, enum proc_type);
void proc_setup(struct proc *, on_read2_fn_t *, on_eof2_fn_t *,
                on_error2_fn_t *, on_exit_fn_t *);
void proc_start(struct proc *, char const *args[]);
void proc_send(struct proc *, char const *string);
void proc_stop(struct proc *);
void proc_cleanup(struct proc *);
bool proc_exitted(struct proc const *);

#endif

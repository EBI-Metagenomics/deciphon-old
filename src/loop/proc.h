#ifndef LOOP_PROC_H
#define LOOP_PROC_H

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
        struct child *child;
    };
};

void proc_init(struct proc *, enum proc_type);
void proc_setup(struct proc *, on_read2_fn_t *, on_eof2_fn_t *,
                on_error2_fn_t *, on_exit3_fn_t *);
void proc_open(struct proc *, char const *args[]);
void proc_send(struct proc *, char const *string);
void proc_close(struct proc *);
bool proc_offline(struct proc const *);
void proc_cleanup(struct proc *);

#endif

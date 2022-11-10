#include "loop/proc.h"

void proc_init(struct proc *proc, enum proc_type type) { proc->type = type; }

void proc_setup(struct proc *p, on_read2_fn_t *onrd, on_eof2_fn_t *oneof,
                on_error2_fn_t *onerr, on_exit_fn_t *onexi)
{
    if (p->type == PROC_PARENT)
        parent_init(&p->parent, onrd, oneof, onerr, onexi);
    if (p->type == PROC_CHILD) child_init(&p->child, onrd, oneof, onerr, onexi);
}

void proc_start(struct proc *proc, char const *args[])
{
    if (proc->type == PROC_PARENT) parent_start(&proc->parent);
    if (proc->type == PROC_CHILD) child_start(&proc->child, args);
}

void proc_send(struct proc *proc, char const *string)
{
    if (proc->type == PROC_PARENT) parent_send(&proc->parent, string);
    if (proc->type == PROC_CHILD) child_send(&proc->child, string);
}

void proc_stop(struct proc *proc)
{
    if (proc->type == PROC_PARENT) parent_stop(&proc->parent);
    if (proc->type == PROC_CHILD) child_stop(&proc->child);
}

void proc_cleanup(struct proc *proc)
{
    if (proc->type == PROC_PARENT) parent_cleanup(&proc->parent);
    if (proc->type == PROC_CHILD) child_cleanup(&proc->child);
}

bool proc_exitted(struct proc const *proc)
{
    if (proc->type == PROC_PARENT) return parent_exitted(&proc->parent);
    if (proc->type == PROC_CHILD) return child_exitted(&proc->child);
    __builtin_unreachable();
}

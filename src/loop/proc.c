#include "loop/proc.h"
#include "loop/child.h"

void proc_init(struct proc *proc, enum proc_type type) { proc->type = type; }

void proc_setup(struct proc *p, on_read2_fn_t *on_read, on_eof2_fn_t *on_eof,
                on_error2_fn_t *on_error, on_exit3_fn_t *on_exit)
{
    if (p->type == PROC_PARENT)
        parent_init(&p->parent, on_read, on_eof, on_error, on_exit);
    if (p->type == PROC_CHILD)
        p->child = child_new(on_read, on_eof, on_error, on_exit);
}

void proc_open(struct proc *proc, char const *args[])
{
    if (proc->type == PROC_PARENT) parent_open(&proc->parent);
    if (proc->type == PROC_CHILD) child_spawn(proc->child, args);
}

void proc_send(struct proc *proc, char const *string)
{
    if (proc->type == PROC_PARENT) parent_send(&proc->parent, string);
    if (proc->type == PROC_CHILD) child_send(proc->child, string);
}

void proc_close(struct proc *proc)
{
    if (proc->type == PROC_PARENT) parent_close(&proc->parent);
    if (proc->type == PROC_CHILD) child_kill(proc->child);
}

bool proc_offline(struct proc const *proc)
{
    if (proc->type == PROC_PARENT) return parent_closed(&proc->parent);
    if (proc->type == PROC_CHILD) return child_closed(proc->child);
    __builtin_unreachable();
}

void proc_cleanup(struct proc *proc)
{
    if (proc->type == PROC_PARENT) parent_close(&proc->parent);
    if (proc->type == PROC_CHILD) child_del(proc->child);
}

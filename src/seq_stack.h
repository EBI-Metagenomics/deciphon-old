#ifndef SEQ_STACK_H
#define SEQ_STACK_H

#include "containers/iter.h"
#include "containers/stack.h"
#include <stdbool.h>

struct seq_stack
{
    struct stack stack;
};

bool              seq_stack_empty(struct seq_stack const* stack);
struct iter_snode seq_stack_iter(struct seq_stack* stack);
void              seq_stack_init(struct seq_stack* stack);
struct seq*       seq_stack_pop(struct seq_stack* stack);
void              seq_stack_push(struct seq_stack* stack, struct seq* seq);

#endif

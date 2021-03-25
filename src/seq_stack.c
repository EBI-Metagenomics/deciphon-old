#include "seq_stack.h"
#include "seq.h"

void seq_stack_deinit(struct seq_stack* stack) { stack_deinit(&stack->stack); }

bool seq_stack_empty(struct seq_stack const* stack) { return stack_empty(&stack->stack); }

struct iter_snode seq_stack_iter(struct seq_stack* stack) { return stack_iter(&stack->stack); }

void seq_stack_init(struct seq_stack* stack) { stack_init(&stack->stack); }

struct seq* seq_stack_pop(struct seq_stack* stack)
{
    struct snode* node = stack_empty(&stack->stack) ? stack_pop(&stack->stack) : NULL;
    return CONTAINER_OF_OR_NULL(node, struct seq, node);
}

void seq_stack_push(struct seq_stack* stack, struct seq* seq) { stack_push(&stack->stack, &seq->node); }

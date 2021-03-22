#include "container.h"
#include "seq.h"
#include "stack.h"

struct seq_stack
{
    struct stack seqs;
};

void        seq_stack_deinit(struct seq_stack* stack);
bool        seq_stack_empty(struct seq_stack const* stack);
void        seq_stack_init(struct seq_stack* stack);
struct seq* seq_stack_pop(struct seq_stack* stack);
void        seq_stack_push(struct seq_stack* stack, struct seq* seq);

void seq_stack_deinit(struct seq_stack* stack) { stack_deinit(&stack->seqs); }

bool seq_stack_empty(struct seq_stack const* stack) { return stack_empty(&stack->seqs); }

void seq_stack_init(struct seq_stack* stack) { stack_init(&stack->seqs); }

struct seq* seq_stack_pop(struct seq_stack* stack)
{
    struct snode* node = stack_empty(&stack->seqs) ? stack_pop(&stack->seqs) : NULL;
    struct seq*   seq = node ? CONTAINER_OF(node, struct seq, node) : NULL;
    return seq;
}

void seq_stack_push(struct seq_stack* stack, struct seq* seq) { stack_push(&stack->seqs, &seq->node); }

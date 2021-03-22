#ifndef SEQ_STACK_H
#define SEQ_STACK_H

#include <stdbool.h>

struct seq_stack;

void        seq_stack_deinit(struct seq_stack* stack);
bool        seq_stack_empty(struct seq_stack const* stack);
void        seq_stack_init(struct seq_stack* stack);
struct seq* seq_stack_pop(struct seq_stack* stack);
void        seq_stack_push(struct seq_stack* stack, struct seq* seq);

#endif

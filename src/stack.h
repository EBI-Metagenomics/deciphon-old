#ifndef STACK_H
#define STACK_H

#include "snode.h"
#include <stdbool.h>

struct stack
{
    struct snode head;
};

#define STACK_INIT(name)                                                                                               \
    {                                                                                                                  \
        SNODE_INIT()                                                                                                   \
    }

static inline void          stack_deinit(struct stack* stack);
static inline bool          stack_empty(struct stack const* stack);
static inline void          stack_init(struct stack* stack);
static inline struct snode* stack_pop(struct stack* stack);
static inline void          stack_push(struct stack* stack, struct snode* new);

static inline void stack_deinit(struct stack* stack) { snode_deinit(&stack->head); }

static inline bool stack_empty(struct stack const* stack) { return stack->head.next == NULL; }

static inline void stack_init(struct stack* stack) { snode_init(&stack->head); }

static inline struct snode* stack_pop(struct stack* stack)
{
    if (stack_empty(stack))
        return NULL;

    struct snode* node = snode_next(&stack->head);
    snode_del(&stack->head, node);
    return node;
}

static inline void stack_push(struct stack* stack, struct snode* new) { snode_add_next(&stack->head, new); }

#endif

#ifndef FIFO1_H
#define FIFO1_H

#include <ck_fifo.h>
#include <stdbool.h>

struct fifo1;

struct fifo1_node
{
    struct ck_fifo_spsc_entry entry;
};

void               fifo1_close(struct fifo1* fifo);
bool               fifo1_closed(struct fifo1* fifo);
struct fifo1*      fifo1_create(void);
void               fifo1_destroy(struct fifo1* fifo);
bool               fifo1_empty(struct fifo1* fifo);
struct fifo1_node* fifo1_pop(struct fifo1* fifo);
void               fifo1_push(struct fifo1* fifo, struct fifo1_node* new);

#endif

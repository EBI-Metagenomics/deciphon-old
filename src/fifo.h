#ifndef FIFO_H
#define FIFO_H

#include "containers/queue.h"
#include <stdbool.h>

struct fifo;

void          fifo_close(struct fifo* fifo);
bool          fifo_closed(struct fifo* fifo);
struct fifo*  fifo_create(void);
void          fifo_destroy(struct fifo* fifo);
bool          fifo_empty(struct fifo* fifo);
struct snode* fifo_pop(struct fifo* fifo);
void          fifo_push(struct fifo* fifo, struct snode* new);

#endif

#ifndef QUEUE_H
#define QUEUE_H

#include <stdint.h>

struct queue;
struct task;

struct queue* queue_create(uint32_t max_size);
void          queue_destroy(struct queue* queue);
void          queue_finish(struct queue* queue);
struct task*  queue_pop(struct queue* queue);
void          queue_push(struct queue* queue, struct task* task);

#endif

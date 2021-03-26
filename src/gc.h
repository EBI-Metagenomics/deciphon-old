#ifndef GC_H
#define GC_H

#include <stdbool.h>

struct gc;

typedef void (*gc_collect_cb)(void* arg);

void       gc_collect(struct gc* gc);
struct gc* gc_create(gc_collect_cb collect, void* collect_arg);
void       gc_destroy(struct gc* gc);
void       gc_join(struct gc* gc);
void       gc_start(struct gc* gc);
void       gc_stop(struct gc* gc);

#endif

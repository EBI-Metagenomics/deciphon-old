#ifndef DECIPHON_LOOP_WRITER_H
#define DECIPHON_LOOP_WRITER_H

#include "uv.h"
#include <stdbool.h>

struct looper;
struct writer;

struct writer *writer_new(struct looper *looper, uv_file fd);
void writer_put(struct writer *writer, char const *msg);
void writer_del(struct writer *writer);

#endif

#ifndef CHARARRAY_H
#define CHARARRAY_H

#include <stddef.h>

struct chararray
{
  size_t size;
  size_t capacity;
  char *data;
};

void chararray_init(struct chararray *);
void chararray_cleanup(struct chararray *);

size_t chararray_size(struct chararray const *);
char *chararray_data(struct chararray *);

int chararray_append(struct chararray *, char);
void chararray_reset(struct chararray *);

#endif

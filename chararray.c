#include "chararray.h"
#include "include/deciphon/errno.h"
#include <stdlib.h>

void chararray_init(struct chararray *x)
{
  x->size = 0;
  x->capacity = 0;
  x->data = NULL;
}

void chararray_cleanup(struct chararray *x)
{
  if (x)
  {
    free(x->data);
    x->data = NULL;
    x->size = 0;
    x->capacity = 0;
  }
}

size_t chararray_size(struct chararray const *x) { return x->size; }

char *chararray_data(struct chararray *x) { return x->data; }

static size_t next_capacity(size_t size);

int chararray_append(struct chararray *x, char c)
{
  if (x->size + 1 > x->capacity)
  {
    char *data = realloc(x->data, next_capacity(x->capacity));
    if (!data) return DCP_ENOMEM;
    x->capacity = next_capacity(x->capacity);
    x->data = data;
  }

  x->data[x->size++] = c;

  return 0;
}

void chararray_reset(struct chararray *x) { x->size = 0; }

static size_t next_capacity(size_t x)
{
  size_t const mininc = 32;
  size_t const maxinc = 16 * 1024 * 1024;

  if (x < mininc) return x + mininc;
  if (x > maxinc) return x + maxinc;

  return 2 * x;
}

#include "chararray.h"
#include "include/deciphon/errno.h"
#include <stdlib.h>

void chararray_init(struct chararray *x)
{
  x->size = 0;
  x->data = NULL;
}

void chararray_cleanup(struct chararray *x)
{
  if (x->data) free(x->data);
  x->size = 0;
  x->data = NULL;
}

size_t chararray_size(struct chararray const *x) { return x->size; }

char *chararray_data(struct chararray *x) { return x->data; }

static size_t next_size(size_t size);

int chararray_append(struct chararray *x, char c)
{
  char *data = realloc(x->data, next_size(x->size));
  if (!data) return DCP_ENOMEM;

  x->data = data;
  x->data[x->size] = c;
  x->size = next_size(x->size);

  return 0;
}

void chararray_reset(struct chararray *x) { x->size = 0; }

static size_t next_size(size_t size)
{
  size_t const mininc = 32;
  size_t const maxinc = 16 * 1024 * 1024;

  if (size < mininc) return size + mininc;
  if (size > maxinc) return size + maxinc;

  return 2 * size;
}

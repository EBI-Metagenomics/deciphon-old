#include "protein_iter.h"
#include "protein.h"
#include "protein_reader.h"

void protein_iter_init(struct protein_iter *x, struct protein_reader *reader,
                       int partition, int protein_offset)
{
  x->partition = partition;
  x->protein_offset = protein_offset;
  x->protein_idx = 0;
  x->reader = reader;
}

int protein_iter_rewind(struct protein_iter *x)
{
  x->protein_idx = 0;
  return protein_reader_rewind(x->reader, x->partition);
}

int protein_iter_next(struct protein_iter *x, struct protein **y)
{
  int rc = protein_reader_next(x->reader, x->partition, y);
  x->protein_idx += !protein_reader_end(x->reader, x->partition);
  return rc;
}

bool protein_iter_end(struct protein_iter const *x)
{
  return protein_reader_end(x->reader, x->partition);
}

int protein_iter_idx(struct protein_iter const *x)
{
  return x->protein_offset + x->protein_idx;
}

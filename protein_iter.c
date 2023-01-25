#include "protein_iter.h"
#include "protein.h"
#include "protein_reader.h"

void protein_iter_init(struct protein_iter *x, struct protein_reader *reader,
                       unsigned partition)
{
  x->partition = partition;
  x->reader = reader;
}

int protein_iter_rewind(struct protein_iter *x)
{
  return protein_reader_rewind(x->reader, x->partition);
}

int protein_iter_next(struct protein_iter *x, struct protein **y)
{
  return protein_reader_next(x->reader, x->partition, y);
}

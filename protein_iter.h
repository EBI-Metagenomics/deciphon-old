#ifndef PROTEIN_ITER_H
#define PROTEIN_ITER_H

#include <stdbool.h>

struct protein;
struct protein_reader;

struct protein_iter
{
  int partition;
  int protein_offset;
  int protein_idx;
  struct protein_reader *reader;
};

void protein_iter_init(struct protein_iter *, struct protein_reader *,
                       int partition, int protein_offset);
int protein_iter_rewind(struct protein_iter *);
int protein_iter_next(struct protein_iter *, struct protein **);
bool protein_iter_end(struct protein_iter const *);
int protein_iter_idx(struct protein_iter const *);

#endif

#ifndef PROTEIN_ITER_H
#define PROTEIN_ITER_H

struct protein;
struct protein_reader;

struct protein_iter
{
  int partition;
  int protein_idx;
  struct protein_reader *reader;
};

void protein_iter_init(struct protein_iter *, struct protein_reader *,
                       int partition, int protein_idx);
int protein_iter_rewind(struct protein_iter *);
int protein_iter_next(struct protein_iter *, struct protein **);

#endif

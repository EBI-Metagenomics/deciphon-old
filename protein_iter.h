#ifndef PROTEIN_ITER_H
#define PROTEIN_ITER_H

#include "lip/lip.h"
#include "protein.h"
#include <stdbool.h>

struct protein_reader;

struct protein_iter
{
  int partition;
  int start_idx;
  int curr_idx;
  long offset;
  FILE *fp;
  struct lip_file file;
  struct protein_reader *reader;
};

void protein_iter_init(struct protein_iter *, struct protein_reader *,
                       int partition, int start_idx, long offset, FILE *);
int protein_iter_rewind(struct protein_iter *);
int protein_iter_next(struct protein_iter *, struct protein *);
bool protein_iter_end(struct protein_iter const *);
int protein_iter_idx(struct protein_iter const *);

#endif

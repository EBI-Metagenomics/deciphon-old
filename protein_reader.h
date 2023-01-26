#ifndef PROTEIN_READER_H
#define PROTEIN_READER_H

#include "deciphon/limits.h"

struct db_reader;
struct protein_iter;

struct protein_reader
{
  int npartitions;
  int partition_csum[DCP_NPARTITIONS_MAX + 1];
  long partition_offset[DCP_NPARTITIONS_MAX + 1];
  struct db_reader *db;
};

void protein_reader_init(struct protein_reader *);
int protein_reader_setup(struct protein_reader *, struct db_reader *,
                         int npartitions);

int protein_reader_npartitions(struct protein_reader const *);
int protein_reader_partition_size(struct protein_reader const *, int partition);
int protein_reader_size(struct protein_reader const *);
int protein_reader_iter(struct protein_reader *, int partition,
                        struct protein_iter *);

#endif

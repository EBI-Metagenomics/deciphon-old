#ifndef PROTEIN_READER_H
#define PROTEIN_READER_H

#include "lite_pack/lite_pack.h"
#include "npartitions_max.h"
#include "protein.h"

struct protein_reader
{
  unsigned npartitions;
  unsigned partition_size[NPARTITIONS_MAX];
  long partition_offset[NPARTITIONS_MAX + 1];
  struct lip_file file[NPARTITIONS_MAX];
  long curr_offset[NPARTITIONS_MAX];
  struct protein proteins[NPARTITIONS_MAX];
};

struct db_reader;

int protein_reader_setup(struct protein_reader *, struct db_reader *,
                         unsigned npartitions);
unsigned protein_reader_npartitions(struct protein_reader const *);
unsigned protein_reader_partition_size(struct protein_reader const *,
                                       unsigned partition);
unsigned protein_reader_nprofiles(struct protein_reader const *);
int protein_reader_next(struct protein_reader *, unsigned partition,
                        struct protein **);
bool protein_reader_end(struct protein_reader const *, unsigned partition);
void protein_reader_del(struct protein_reader *);

#endif

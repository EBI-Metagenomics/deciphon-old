#ifndef PROTEIN_READER_H
#define PROTEIN_READER_H

#include "deciphon/limits.h"
#include "lip/lip.h"
#include "protein.h"
#include <stdio.h>

struct protein_reader
{
  int npartitions;
  int partition_size[DCP_NPARTITIONS_SIZE];
  long partition_offset[DCP_NPARTITIONS_SIZE + 1];
  FILE *fp[DCP_NPARTITIONS_SIZE];
  struct lip_file file[DCP_NPARTITIONS_SIZE];
  long curr_offset[DCP_NPARTITIONS_SIZE];
  struct protein proteins[DCP_NPARTITIONS_SIZE];
};

struct db_reader;

void protein_reader_init(struct protein_reader *);
int protein_reader_open(struct protein_reader *, struct db_reader *,
                        int npartitions);
void protein_reader_close(struct protein_reader *);

int protein_reader_npartitions(struct protein_reader const *);
int protein_reader_partition_size(struct protein_reader const *, int partition);
int protein_reader_size(struct protein_reader const *);
int protein_reader_rewind(struct protein_reader *, int partition);
int protein_reader_next(struct protein_reader *, int partition,
                        struct protein **);
bool protein_reader_end(struct protein_reader const *, int partition);

#endif

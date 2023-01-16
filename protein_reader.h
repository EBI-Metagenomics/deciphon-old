#ifndef PROTEIN_READER_H
#define PROTEIN_READER_H

#include "deciphon_limits.h"
#include "lite_pack/lite_pack.h"
#include "protein.h"
#include <stdint.h>

struct protein_reader
{
  unsigned npartitions;
  unsigned partition_size[PARTITIONS_MAX];
  long partition_offset[PARTITIONS_MAX + 1];
  struct lip_file file[PARTITIONS_MAX];
  long curr_offset[PARTITIONS_MAX];
  struct protein profiles[PARTITIONS_MAX];
};

struct db_reader;

int protein_reader_setup(struct protein_reader *reader, struct db_reader *db,
                         unsigned npartitions);
unsigned protein_reader_npartitions(struct protein_reader const *reader);
unsigned protein_reader_partition_size(struct protein_reader const *reader,
                                       unsigned partition);
unsigned protein_reader_nprofiles(struct protein_reader const *reader);
int protein_reader_rewind_all(struct protein_reader *reader);
int protein_reader_rewindb(struct protein_reader *reader, unsigned partition);
int protein_reader_next(struct protein_reader *reader, unsigned partition,
                        struct protein **profile);
bool protein_reader_end(struct protein_reader const *reader,
                        unsigned partition);
void protein_reader_del(struct protein_reader *reader);

#endif

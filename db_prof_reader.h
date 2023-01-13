#ifndef DB_PROF_READER_H
#define DB_PROF_READER_H

#include "deciphon_limits.h"
#include "lite_pack/lite_pack.h"
#include "prof_typeid.h"
#include "prot_prof.h"
#include <stdint.h>

struct prof_reader
{
  unsigned npartitions;
  unsigned partition_size[PARTITIONS_MAX];
  long partition_offset[PARTITIONS_MAX + 1];
  struct lip_file file[PARTITIONS_MAX];
  enum prof_typeid prof_typeid;
  long curr_offset[PARTITIONS_MAX];
  union
  {
    // struct standard_profile std;
    struct prot_prof pro;
  } profiles[PARTITIONS_MAX];
};

struct db_reader;

int prof_reader_setup(struct prof_reader *reader, struct db_reader *db,
                      unsigned npartitions);
unsigned prof_reader_npartitions(struct prof_reader const *reader);
unsigned prof_reader_partition_size(struct prof_reader const *reader,
                                    unsigned partition);
unsigned prof_reader_nprofiles(struct prof_reader const *reader);
int prof_reader_rewind_all(struct prof_reader *reader);
int prof_reader_rewind(struct prof_reader *reader, unsigned partition);
int prof_reader_next(struct prof_reader *reader, unsigned partition,
                     struct prof **profile);
bool prof_reader_end(struct prof_reader const *reader, unsigned partition);
void prof_reader_del(struct prof_reader *reader);

#endif

#ifndef DECIPHON_PROFILE_READER_H
#define DECIPHON_PROFILE_READER_H

#include "deciphon/limits.h"
#include "deciphon/lite_pack.h"
#include "deciphon/model/model.h"
#include <stdint.h>

struct profile_reader
{
    unsigned npartitions;
    unsigned partition_size[NUM_PARTITIONS];
    int64_t partition_offset[NUM_PARTITIONS + 1];
    struct lip_file file[NUM_PARTITIONS];
    enum profile_typeid profile_typeid;
    union
    {
        // struct standard_profile std;
        struct protein_profile pro;
    } profiles[NUM_PARTITIONS];
};

struct db_reader;

enum rc profile_reader_setup(struct profile_reader *reader,
                             struct db_reader *db, unsigned npartitions);
unsigned profile_reader_npartitions(struct profile_reader const *reader);
unsigned profile_reader_partition_size(struct profile_reader const *reader,
                                       unsigned partition);
enum rc profile_reader_rewind(struct profile_reader *reader);
enum rc profile_reader_next(struct profile_reader *reader, unsigned partition,
                            struct profile **profile);
bool profile_reader_end(struct profile_reader *reader, unsigned partition);
void profile_reader_del(struct profile_reader *reader);

#endif

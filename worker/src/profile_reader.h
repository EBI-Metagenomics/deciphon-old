#ifndef PROFILE_READER_H
#define PROFILE_READER_H

#include "cmp/cmp.h"
#include "common/limits.h"
#include "profile_types.h"
#include "protein_profile.h"
#include "common/rc.h"
#include "standard_profile.h"
#include <stdbool.h>
#include <stdio.h>

struct profile_reader
{
    unsigned npartitions;
    unsigned partition_size[NUM_PARTITIONS];
    off_t partition_offset[NUM_PARTITIONS + 1];
    struct cmp_ctx_s cmp[NUM_PARTITIONS];
    enum profile_typeid profile_typeid;
    union
    {
        struct standard_profile std;
        struct protein_profile pro;
    } profiles[NUM_PARTITIONS];
};

struct db;

enum rc profile_reader_setup(struct profile_reader *reader, struct db *db,
                             unsigned npartitions);
unsigned profile_reader_npartitions(struct profile_reader const *reader);
unsigned profile_reader_partition_size(struct profile_reader const *reader,
                                       unsigned partition);
enum rc profile_reader_rewind(struct profile_reader *reader);
enum rc profile_reader_next(struct profile_reader *reader, unsigned partition,
                            struct profile **profile);
bool profile_reader_end(struct profile_reader *reader, unsigned partition);
void profile_reader_del(struct profile_reader *reader);

#endif

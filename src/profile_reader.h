#ifndef PROFILE_READER_H
#define PROFILE_READER_H

#include "cmp/cmp.h"
#include "dcp_limits.h"
#include "profile_types.h"
#include "protein_profile.h"
#include "rc.h"
#include "standard_profile.h"
#include <stdbool.h>
#include <stdio.h>

struct profile_reader
{
    unsigned npartitions;
    off_t partition_offset[DCP_NUM_PARTITIONS + 1];
    struct cmp_ctx_s cmp[DCP_NUM_PARTITIONS];
    enum profile_typeid profile_typeid;
    union
    {
        struct standard_profile std;
        struct protein_profile pro;
    } profiles[DCP_NUM_PARTITIONS];
};

struct db;

enum rc profile_reader_setup(struct profile_reader *reader, struct db *db,
                             unsigned npartitions);
unsigned profile_reader_npartitions(struct profile_reader const *reader);
enum rc profile_reader_rewind(struct profile_reader *reader);
enum rc profile_reader_next(struct profile_reader *reader, unsigned partition);
struct profile *profile_reader_profile(struct profile_reader *reader,
                                       unsigned partition);
bool profile_reader_end(struct profile_reader *reader, unsigned partition);

#endif

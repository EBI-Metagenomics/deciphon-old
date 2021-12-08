#ifndef PROFILE_READER_H
#define PROFILE_READER_H

#include "cmp/cmp.h"
#include "dcp_limits.h"
#include "rc.h"
#include <stdbool.h>
#include <stdio.h>

struct profile_reader
{
    unsigned npartitions;
    off_t partition_offset[DCP_NUM_PARTITIONS + 1];
    FILE *fp[DCP_NUM_PARTITIONS];
    struct cmp_ctx_s cmp[DCP_NUM_PARTITIONS];
};

struct db;

enum rc profile_reader_setup(struct profile_reader *reader, struct db *db,
                             unsigned npartitions);
unsigned profile_reader_npartitions(struct profile_reader const *reader);
void profile_reader_rewind(struct profile_reader *reader);
enum rc profile_reader_next(struct profile_reader *reader, unsigned fd);
bool profile_reader_end(struct profile_reader *reader, unsigned fd);

#endif

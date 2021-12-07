#ifndef PROFILE_READER_H
#define PROFILE_READER_H

#include "rc.h"
#include <stdbool.h>
#include <stdio.h>

struct profile_reader
{
    unsigned nfiles;
    FILE *fp[64];
};

/* db_init_profile_reader(&reader, 4); */
unsigned profile_reader_nfiles(struct profile_reader const *reader);
void profile_reader_rewind(struct profile_reader *reader);
enum rc profile_reader_next(struct profile_reader *reader, unsigned fd);
bool profile_reader_end(struct profile_reader *reader, unsigned fd);

#endif

#include "profile_reader.h"
#include "db.h"

enum rc profile_reader_setup(struct profile_reader *reader, struct db *db,
                             unsigned npartitions)
{
    reader->npartitions = npartitions;
}

unsigned profile_reader_npartitions(struct profile_reader const *reader)
{
    return reader->npartitions;
}

void profile_reader_rewind(struct profile_reader *reader)
{
    for (unsigned i = 0; i < reader->npartitions; ++i)
        rewind(reader->fp[i]);
}

enum rc profile_reader_next(struct profile_reader *reader, unsigned fd)
{
    return RC_DONE;
}

bool profile_reader_end(struct profile_reader *reader, unsigned fd)
{
    return true;
}

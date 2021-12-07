#include "profile_reader.h"

unsigned profile_reader_nfiles(struct profile_reader const *reader)
{
    return reader->nfiles;
}

void profile_reader_rewind(struct profile_reader *reader)
{
    for (unsigned i = 0; i < reader->nfiles; ++i)
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

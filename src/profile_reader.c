#include "profile_reader.h"
#include "db.h"
#include "logger.h"
#include "xfile.h"
#include "xmath.h"

static void cleanup(struct profile_reader *reader)
{
    for (unsigned i = 0; i < reader->npartitions; ++i)
        fclose(cmp_file(reader->cmp + i));
}

static enum rc open_files(struct profile_reader *reader, struct db *db,
                          unsigned npartitions)
{
    reader->npartitions = 0;
    for (unsigned i = 0; i < npartitions; ++i)
    {
        FILE *fp = xfile_open_from_fptr(cmp_file(&db->file.cmp), "rb");
        if (!fp)
        {
            enum rc rc = error(RC_IOERROR, "failed to open file");
            if (rc) return rc;
        }
        cmp_setup(reader->cmp + i, fp);
        reader->npartitions++;
    }
    return RC_DONE;
}

static enum rc partition_it(struct profile_reader *reader, struct db *db)
{
    enum rc rc = RC_DONE;
    struct profile *prof = (struct profile *)&reader->profiles[0];
    struct cmp_ctx_s *cmp = &reader->cmp[0];
    FILE *fp = cmp_file(cmp);
    reader->partition_offset[0] = db_profiles_block_offset(db);
    unsigned part_idx = 1;
    unsigned part_size = 0;
    while (part_idx < reader->npartitions)
    {
        if ((rc = profile_read(prof, cmp))) goto cleanup;

        part_size++;
        unsigned n = db_nprofiles(db);
        unsigned m = reader->npartitions;
        if (part_size >= xmath_partition_size(n, m, part_idx - 1))
        {
            if ((reader->partition_offset[++part_idx] = ftello(fp)) == -1)
            {
                rc = error(RC_IOERROR, "failed to ftello");
                goto cleanup;
            }

            part_size = 0;
        }
    }

    while ((rc = profile_read(prof, cmp)) == RC_NEXT)
        ;
    if (rc) goto cleanup;

    if ((reader->partition_offset[++part_idx] = ftello(fp)) == -1)
    {
        rc = error(RC_IOERROR, "failed to ftello");
        goto cleanup;
    }

    return RC_DONE;

cleanup:
    cleanup(reader);
    return rc;
}

enum rc profile_reader_setup(struct profile_reader *reader, struct db *db,
                             unsigned npartitions)
{
    if (npartitions > DCP_NUM_PARTITIONS)
        return error(RC_ILLEGALARG, "too many partitions");

    if (db_nprofiles(db) > npartitions) npartitions = db_nprofiles(db);

    enum rc rc = RC_DONE;
    if ((rc = open_files(reader, db, npartitions))) goto cleanup;
    if ((rc = partition_it(reader, db))) goto cleanup;
    reader->profile_typeid = (enum profile_typeid)db_profile_typeid(db);
    return rc;

cleanup:
    cleanup(reader);
    return rc;
}

unsigned profile_reader_npartitions(struct profile_reader const *reader)
{
    return reader->npartitions;
}

void profile_reader_rewind(struct profile_reader *reader)
{
    for (unsigned i = 0; i < reader->npartitions; ++i)
        rewind(cmp_file(reader->cmp + i));
}

enum rc profile_reader_next(struct profile_reader *reader, unsigned partition)
{
    struct profile *prof = profile_reader_profile(reader, partition);
    return profile_read(prof, &reader->cmp[partition]);
}

struct profile *profile_reader_profile(struct profile_reader *reader,
                                       unsigned partition)
{
    return (struct profile *)&reader->profiles[partition];
}

bool profile_reader_end(struct profile_reader *reader, unsigned partition)
{
    return true;
}

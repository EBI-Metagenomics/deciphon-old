#include "profile_reader.h"
#include "common/logger.h"
#include "common/xfile.h"
#include "common/xmath.h"
#include "db.h"
#include "protein_db.h"
#include "standard_db.h"

static void cleanup(struct profile_reader *reader)
{
    for (unsigned i = 0; i < reader->npartitions; ++i)
        fclose(cmp_file(reader->cmp + i));
}

static enum rc open_files(struct profile_reader *reader, struct db *db)
{
    for (unsigned i = 0; i < reader->npartitions; ++i)
    {
        FILE *fp = xfile_open_from_fptr(cmp_file(&db->file.cmp), "rb");
        if (!fp)
        {
            enum rc rc = error(RC_EIO, "failed to open file");
            if (rc) return rc;
        }
        cmp_setup(reader->cmp + i, fp);
    }
    return RC_DONE;
}

static void init_standard_profiles(struct profile_reader *reader,
                                   struct standard_db *db)
{
    for (unsigned i = 0; i < reader->npartitions; ++i)
        standard_profile_init(&reader->profiles[i].std, &db->code);
}

static void init_protein_profiles(struct profile_reader *reader,
                                  struct protein_db *db)
{
    for (unsigned i = 0; i < reader->npartitions; ++i)
        protein_profile_init(&reader->profiles[i].pro, &db->amino, &db->code,
                             db->cfg);
}

static void setup_profile_indices(struct profile_reader *reader)
{
    int offset = -1;
    for (unsigned i = 0; i < reader->npartitions; ++i)
    {
        ((struct profile *)&reader->profiles[i])->idx_within_db = offset;
        offset += reader->partition_size[i];
    }
}

static enum rc __rewind(struct profile_reader *reader, unsigned npartitions)
{
    for (unsigned i = 0; i < npartitions; ++i)
    {
        if (cmp_fseek(reader->cmp + i, reader->partition_offset[i], SEEK_SET))
            return error(RC_EIO, "failed to fseek");
    }
    setup_profile_indices(reader);
    return RC_DONE;
}

static enum rc record_offset(struct cmp_ctx_s *cmp, int64_t *offset)
{
    if ((*offset = cmp_ftell(cmp)) == -1)
        return error(RC_EIO, "failed to ftello");
    return RC_DONE;
}

static enum rc partition_it(struct profile_reader *reader, struct db *db)
{
    enum rc rc = RC_DONE;
    reader->partition_offset[0] = db_profiles_block_offset(db);
    if ((rc = __rewind(reader, 1))) return rc;

    struct cmp_ctx_s *cmp = &reader->cmp[0];
    unsigned npartitions = reader->npartitions;
    unsigned nprofiles = db_nprofiles(db);
    unsigned i = 0;
    unsigned size = 0;
    struct profile *prof = (struct profile *)&reader->profiles[0];
    for (unsigned j = 0; j < nprofiles; ++j)
    {
        if ((rc = profile_read(prof, cmp))) goto cleanup;

        size++;
        if (size >= xmath_partition_size(nprofiles, npartitions, i))
        {
            reader->partition_size[i] = size;
            if ((rc = record_offset(cmp, reader->partition_offset + ++i)))
                goto cleanup;
            size = 0;
        }
    }
    assert(i == npartitions);
    if ((rc = record_offset(cmp, reader->partition_offset + npartitions)))
        goto cleanup;

    return RC_DONE;

cleanup:
    cleanup(reader);
    return rc;
}

enum rc profile_reader_setup(struct profile_reader *reader, struct db *db,
                             unsigned npartitions)
{
    if (npartitions == 0) return error(RC_EINVAL, "can't have zero partitions");

    if (npartitions > NUM_PARTITIONS)
        return error(RC_EINVAL, "too many partitions");

    printf("Number of profiles: %d\n", db_nprofiles(db));
    if (db_nprofiles(db) < npartitions) npartitions = db_nprofiles(db);
    reader->npartitions = npartitions;

    enum rc rc = RC_DONE;
    reader->profile_typeid = (enum profile_typeid)db_profile_typeid(db);
    if ((rc = open_files(reader, db))) goto cleanup;

    if (reader->profile_typeid == PROFILE_STANDARD)
        init_standard_profiles(reader, (struct standard_db *)db);
    else if (reader->profile_typeid == PROFILE_PROTEIN)
        init_protein_profiles(reader, (struct protein_db *)db);
    else
        assert(false);

    if ((rc = partition_it(reader, db))) goto cleanup;
    for (unsigned i = 0; i < npartitions; ++i)
    {
        printf("partition_size[%d]: %d\n", i, reader->partition_size[i]);
    }
    for (unsigned i = 0; i < npartitions + 1; ++i)
    {
        printf("partition_offset[%d]: %lld\n", i, reader->partition_offset[i]);
    }
    if ((rc = profile_reader_rewind(reader))) goto cleanup;
    return rc;

cleanup:
    cleanup(reader);
    return rc;
}

unsigned profile_reader_npartitions(struct profile_reader const *reader)
{
    return reader->npartitions;
}

unsigned profile_reader_partition_size(struct profile_reader const *reader,
                                       unsigned partition)
{
    return reader->partition_size[partition];
}

enum rc profile_reader_rewind(struct profile_reader *reader)
{
    return __rewind(reader, reader->npartitions);
}

static enum rc reached_end(struct profile_reader *reader, unsigned partition)
{
    int64_t offset = cmp_ftell(reader->cmp + partition);
    if (offset == -1) return error(RC_EIO, "failed to ftello");
    if (offset == reader->partition_offset[partition + 1]) return RC_END;
    return RC_NEXT;
}

enum rc profile_reader_next(struct profile_reader *reader, unsigned partition,
                            struct profile **profile)
{
    *profile = (struct profile *)&reader->profiles[partition];
    (*profile)->idx_within_db++;
    enum rc rc = reached_end(reader, partition);
    if (rc == RC_NEXT)
    {
        rc = profile_read(*profile, &reader->cmp[partition]);
        if (rc) return rc;
        return RC_NEXT;
    }
    return rc;
}

bool profile_reader_end(struct profile_reader *reader, unsigned partition)
{
    return true;
}

void profile_reader_del(struct profile_reader *reader)
{
    for (unsigned i = 0; i < reader->npartitions; ++i)
        profile_del((struct profile *)&reader->profiles[i]);
}

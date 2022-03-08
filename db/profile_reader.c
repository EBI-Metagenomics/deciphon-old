#include "deciphon/db/db.h"
#include "deciphon/expect.h"
#include "deciphon/logger.h"
#include "deciphon/xfile.h"
#include "deciphon/xmath.h"

static void cleanup(struct profile_reader *reader)
{
    for (unsigned i = 0; i < reader->npartitions; ++i)
        fclose(lip_file_ptr(reader->file + i));
}

static enum rc open_files(struct profile_reader *reader, FILE *fp)
{
    for (unsigned i = 0; i < reader->npartitions; ++i)
    {
        FILE *f = xfile_open_from_fptr(fp, "rb");
        if (!f) return eio("failed to open file");
        lip_file_init(reader->file + i, f);
    }
    return RC_OK;
}

#if 0
static void init_standard_profiles(struct profile_reader *reader,
                                   struct standard_db *db)
{
    for (unsigned i = 0; i < reader->npartitions; ++i)
        standard_profile_init(&reader->profiles[i].std, &db->code);
}
#endif

static void init_protein_profiles(struct profile_reader *reader,
                                  struct protein_db_reader *db)
{
    for (unsigned i = 0; i < reader->npartitions; ++i)
    {
        struct protein_profile *pro = &reader->profiles[i].pro;
        protein_profile_init(pro, "", &db->amino, &db->code, db->cfg);
    }
}

#if 0
static void setup_profile_indices(struct profile_reader *reader)
{
    int offset = -1;
    for (unsigned i = 0; i < reader->npartitions; ++i)
    {
        ((struct profile *)&reader->profiles[i])->idx_within_db = offset;
        offset += reader->partition_size[i];
    }
}
#endif

static void partition_it(struct profile_reader *reader, struct db_reader *db,
                         int64_t offset)
{
    reader->partition_offset[0] = offset;
    // reader->partition_offset[0] = db->profile_offsets[0];

    unsigned nparts = reader->npartitions;
    unsigned nprofs = db->nprofiles;
    unsigned i = 0;
    unsigned size = 0;
    for (unsigned j = 0; j < nprofs; ++j)
    {
        reader->partition_offset[i + 1] += db->profile_sizes[j];

        if (++size >= xmath_partition_size(nprofs, nparts, i))
        {
            reader->partition_size[i] = size;
            reader->partition_offset[i + 1] += reader->partition_offset[i];
            ++i;
            size = 0;
        }
    }
    assert(i == nparts);
    // reader->partition_offset[nparts] = db->profile_offsets[nprofs];
}

enum rc profile_reader_setup(struct profile_reader *reader,
                             struct db_reader *db, unsigned npartitions)
{
    if (npartitions == 0) return einval("can't have zero partitions");

    if (npartitions > NUM_PARTITIONS) return einval("too many partitions");

    printf("Number of profiles: %d\n", db->nprofiles);
    reader->npartitions = xmath_min(npartitions, db->nprofiles);

    if (!expect_map_key(&db->file, "profiles")) return eio("read key");

    unsigned n = 0;
    if (!lip_read_array_size(&db->file, &n)) eio("read array size");
    if (n != db->nprofiles) return einval("invalid nprofiles");

    int64_t profiles_offset = xfile_tell(db->file.fp);

    enum rc rc = RC_OK;
    reader->profile_typeid = db->profile_typeid;
    if ((rc = open_files(reader, lip_file_ptr(&db->file)))) goto cleanup;

    if (reader->profile_typeid == PROFILE_PROTEIN)
        init_protein_profiles(reader, (struct protein_db_reader *)db);
    else
        assert(false);

    partition_it(reader, db, profiles_offset);
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
    for (unsigned i = 0; i < reader->npartitions; ++i)
    {
        FILE *fp = lip_file_ptr(reader->file + i);
        if (!xfile_seek(fp, reader->partition_offset[i], SEEK_SET))
            return eio("failed to fseek");
    }
    return RC_OK;
}

static enum rc reached_end(struct profile_reader *reader, unsigned partition)
{
    int64_t offset = xfile_tell(lip_file_ptr(reader->file + partition));
    if (offset == -1) return eio("failed to ftello");
    if (offset == reader->partition_offset[partition + 1]) return RC_END;
    return RC_OK;
}

enum rc profile_reader_next(struct profile_reader *reader, unsigned partition,
                            struct profile **profile)
{
    *profile = (struct profile *)&reader->profiles[partition];
    // (*profile)->idx_within_db++;
    enum rc rc = reached_end(reader, partition);
    if (rc == RC_OK)
    {
        rc = profile_unpack(*profile, &reader->file[partition]);
        if (rc) return rc;
        return RC_OK;
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
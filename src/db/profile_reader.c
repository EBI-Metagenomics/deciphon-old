#include "db/db.h"
#include "expect.h"
#include "fs.h"
#include "logy.h"
#include "xmath.h"
#include "zc.h"

static void cleanup(struct profile_reader *reader)
{
    for (unsigned i = 0; i < reader->npartitions; ++i)
        fclose(lip_file_ptr(reader->file + i));
}

static enum rc open_files(struct profile_reader *reader, FILE *fp)
{
    for (unsigned i = 0; i < reader->npartitions; ++i)
    {
        FILE *f = NULL;
        int rc = fs_refopen(fp, "rb", &f);
        if (rc) return eio("%s", fs_strerror(rc));
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

static void init_prot_profiles(struct profile_reader *reader,
                               struct prot_db_reader *db)
{
    for (unsigned i = 0; i < reader->npartitions; ++i)
    {
        struct prot_profile *pro = &reader->profiles[i].pro;
        prot_profile_init(pro, "", &db->amino, &db->code, db->cfg);
    }
}

static void partition_init(struct profile_reader *reader, long offset)
{
    long *poffset = reader->partition_offset;
    unsigned *psize = reader->partition_size;

    zc_bzero(poffset, NUM_THREADS + 1);
    zc_bzero(psize, NUM_THREADS);
    poffset[0] = offset;
}

static void partition_it(struct profile_reader *reader, struct db_reader *db)
{
    unsigned n = db->nprofiles;
    unsigned k = reader->npartitions;
    unsigned part = 0;
    for (unsigned i = 0; i < k; ++i)
    {
        long sz = xmath_partition_size(n, k, i);
        assert(sz >= 0);
        assert(sz <= UINT_MAX);
        unsigned size = (unsigned)sz;

        reader->partition_size[i] = size;
        for (unsigned j = 0; j < size; ++j)
            reader->partition_offset[i + 1] += db->profile_sizes[part++];

        reader->partition_offset[i + 1] += reader->partition_offset[i];
    }
}

enum rc profile_reader_setup(struct profile_reader *reader,
                             struct db_reader *db, unsigned npartitions)
{
    if (npartitions == 0) return einval("can't have zero partitions");

    if (npartitions > NUM_THREADS) return einval("too many partitions");

    reader->npartitions = xmath_min(npartitions, db->nprofiles);

    if (!expect_map_key(&db->file, "profiles")) return eio("read key");

    unsigned n = 0;
    if (!lip_read_array_size(&db->file, &n)) eio("read array size");
    if (n != db->nprofiles) return einval("invalid nprofiles");

    long profiles_offset = 0;
    int r = fs_tell(db->file.fp, &profiles_offset);
    if (r) return eio("%s", fs_strerror(r));

    enum rc rc = RC_OK;
    reader->profile_typeid = db->profile_typeid;
    if ((rc = open_files(reader, lip_file_ptr(&db->file)))) goto cleanup;

    if (reader->profile_typeid == PROFILE_PROTEIN)
        init_prot_profiles(reader, (struct prot_db_reader *)db);
    else
        assert(false);

    partition_init(reader, profiles_offset);
    partition_it(reader, db);
    if ((rc = profile_reader_rewind_all(reader))) goto cleanup;
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

unsigned profile_reader_nprofiles(struct profile_reader const *reader)
{
    unsigned n = 0;
    for (unsigned i = 0; i < reader->npartitions; ++i)
        n += reader->partition_size[i];
    return n;
}

enum rc profile_reader_rewind_all(struct profile_reader *reader)
{
    for (unsigned i = 0; i < reader->npartitions; ++i)
    {
        FILE *fp = lip_file_ptr(reader->file + i);
        if (fs_seek(fp, reader->partition_offset[i], SEEK_SET))
            return eio("failed to fseek");
    }
    return RC_OK;
}

enum rc profile_reader_rewind(struct profile_reader *reader, unsigned partition)
{
    FILE *fp = lip_file_ptr(reader->file + partition);
    if (fs_seek(fp, reader->partition_offset[partition], SEEK_SET))
        return eio("failed to fseek");
    return RC_OK;
}

static enum rc reached_end(struct profile_reader *reader, unsigned partition)
{
    long offset = 0;
    if (fs_tell(lip_file_ptr(reader->file + partition), &offset))
        return eio("failed to ftello");
    if (offset == reader->partition_offset[partition + 1]) return RC_END;
    return RC_OK;
}

enum rc profile_reader_next(struct profile_reader *reader, unsigned partition,
                            struct profile **profile)
{
    *profile = (struct profile *)&reader->profiles[partition];
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
    (void)reader;
    (void)partition;
    return true;
}

void profile_reader_del(struct profile_reader *reader)
{
    for (unsigned i = 0; i < reader->npartitions; ++i)
        profile_del((struct profile *)&reader->profiles[i]);
}

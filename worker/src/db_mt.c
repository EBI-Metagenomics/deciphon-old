#include "db_mt.h"
#include "cmp/cmp.h"
#include "common/limits.h"
#include "common/logger.h"
#include "js.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void db_mt_init(struct db_mt *db)
{
    db->offset = 0;
    db->name_length = 0;
    db->size = 0;
    db->data = 0;
}

static void cleanup_metadata_data(struct db_mt *db)
{
    free(db->data);
    db->data = 0;
}

static void cleanup_metadata_parsing(struct db_mt *db)
{
    free(db->offset);
    free(db->name_length);
    db->offset = 0;
    db->name_length = 0;
}

void db_mt_cleanup(struct db_mt *db)
{
    cleanup_metadata_data(db);
    cleanup_metadata_parsing(db);
}

static enum rc read_metadata_data(struct db_mt *db, struct cmp_ctx_s *cmp)
{
    assert(db->size > 0);

    if (!(db->data = malloc(sizeof(char) * db->size)))
        return error(RC_ENOMEM, "failed to alloc for mt.data");

    if (!cmp_read(cmp, db->data, db->size))
    {
        cleanup_metadata_data(db);
        return eio("read metadata");
    }

    if (db->data[db->size - 1])
    {
        cleanup_metadata_data(db);
        return error(RC_EPARSE, "invalid metadata");
    }

    return RC_DONE;
}

static enum rc alloc_metadata_parsing(struct db_mt *db, unsigned nprofiles)
{
    uint32_t n = nprofiles;

    if (!(db->offset = malloc(sizeof(*db->offset) * (n + 1))))
    {
        cleanup_metadata_parsing(db);
        return error(RC_ENOMEM, "failed to alloc for mt.offset");
    }

    if (!(db->name_length = malloc(sizeof(*db->name_length) * n)))
    {
        return error(RC_ENOMEM, "failed to alloc for mt.name_length");
        cleanup_metadata_parsing(db);
    }

    return RC_DONE;
}

static enum rc parse_metadata(struct db_mt *db, unsigned nprofiles)
{
    assert(db->offset);
    assert(db->name_length);

    db->offset[0] = 0;
    for (unsigned i = 0; i < nprofiles; ++i)
    {
        unsigned offset = db->offset[i];
        unsigned j = 0;
        if (offset + j >= db->size)
            return error(RC_EFAIL, "mt.data index overflow");

        /* Name */
        while (db->data[offset + j++])
            ;
        if (j > PROFILE_NAME_SIZE) return error(RC_EINVAL, "name is too long");

        db->name_length[i] = (uint8_t)(j - 1);
        if (offset + j >= db->size)
            return error(RC_EFAIL, "mt.data index overflow");

        /* Accession */
        while (db->data[offset + j++])
            ;
        db->offset[i + 1] = offset + j;
    }

    return RC_DONE;
}

static inline uint32_t max_mt_data_size(void)
{
    return MAX_NPROFILES * (PROFILE_NAME_SIZE + PROFILE_ACC_SIZE);
}

static enum rc read_metadata_size(struct db_mt *db, struct cmp_ctx_s *cmp)
{
    if (!cmp_read_u32(cmp, &db->size)) return eio("read metadata size");

    if (db->size > max_mt_data_size())
        return error(RC_EFAIL, "mt.data size is too big");

    return RC_DONE;
}

enum rc db_mt_read(struct db_mt *db, unsigned nprofiles, struct cmp_ctx_s *cmp)
{
    enum rc rc = RC_DONE;

    if (!JS_XPEC_STR(cmp, "metadata")) eio("skip key");
    uint32_t size = 0;
    cmp_read_bin_size(cmp, &size);
    if (size > max_mt_data_size())
        return error(RC_EFAIL, "mt.data size is too big");
    db->size = size;
    // if ((rc = read_metadata_size(db, cmp))) goto cleanup;

    if (db->size > 0)
    {
        if ((rc = read_metadata_data(db, cmp))) goto cleanup;
        if ((rc = alloc_metadata_parsing(db, nprofiles))) goto cleanup;
        if ((rc = parse_metadata(db, nprofiles))) goto cleanup;
    }

    return RC_DONE;

cleanup:
    cleanup_metadata_data(db);
    cleanup_metadata_parsing(db);
    return rc;
}

static enum rc write_name(struct db_mt *db, struct metadata mt,
                          struct cmp_ctx_s *dst)
{
    if (!cmp_write_str(dst, mt.name, (uint32_t)strlen(mt.name)))
        return eio("write profile name");
    /* +1 for null-terminated string */
    db->size += (uint32_t)strlen(mt.name) + 1;

    return RC_DONE;
}

static enum rc write_accession(struct db_mt *db, struct metadata mt,
                               struct cmp_ctx_s *dst)
{
    if (!cmp_write_str(dst, mt.acc, (uint32_t)strlen(mt.acc)))
        return eio("write profile accession");
    /* +1 for null-terminated string */
    db->size += (uint32_t)strlen(mt.acc) + 1;

    return RC_DONE;
}

enum rc db_mt_write(struct db_mt *db, struct metadata mt, struct cmp_ctx_s *dst)
{
    if (!mt.name) return error(RC_EINVAL, "metadata not set");

    if (strlen(mt.name) >= PROFILE_NAME_SIZE)
        return error(RC_EINVAL, "profile name is too long");

    if (strlen(mt.acc) >= PROFILE_ACC_SIZE)
        return error(RC_EINVAL, "profile accession is too long");

    enum rc rc = RC_DONE;

    if ((rc = write_name(db, mt, dst))) return rc;
    if ((rc = write_accession(db, mt, dst))) return rc;

    return RC_DONE;
}

struct metadata db_mt_metadata(struct db_mt const *db, unsigned idx)
{
    unsigned o = db->offset[idx];
    unsigned size = (unsigned)(db->name_length[idx] + 1);
    return metadata(db->data + o, db->data + o + size);
}

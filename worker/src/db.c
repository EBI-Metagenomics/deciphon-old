#include "db.h"
#include "cmp/cmp.h"
#include "common/compiler.h"
#include "common/limits.h"
#include "common/logger.h"
#include "common/xfile.h"
#include "imm/imm.h"
#include "profile.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* TODO: make sure there is no accession duplicates. */

#define MAGIC_NUMBER 0x765C806BF0E8652B

struct db const db_default = {0};

static enum rc init_tmpmeta(struct db *db)
{
    FILE *fp = tmpfile();
    if (!fp) return error(RC_EIO, "tmpfile() failed");
    cmp_setup(&db->mt.file.cmp, fp);
    return RC_DONE;
}

static enum rc init_tmpdp(struct db *db)
{
    FILE *fp = tmpfile();
    if (!fp) return error(RC_EIO, "tmpfile() failed");
    cmp_setup(&db->dp.cmp, fp);
    return RC_DONE;
}

void db_init(struct db *db, struct db_vtable vtable)
{
    db->vtable = vtable;
    db->float_size = IMM_FLOAT_BYTES;
    db->profiles_block_offset = 0;
    db->profiles.size = 0;
    db->mt.offset = NULL;
    db->mt.name_length = NULL;
    db->mt.size = 0;
    db->mt.data = NULL;
    cmp_setup(&db->mt.file.cmp, NULL);
    cmp_setup(&db->dp.cmp, NULL);
    cmp_setup(&db->file.cmp, NULL);
    db->file.mode = DB_OPEN_NULL;
}

struct imm_abc const *db_abc(struct db const *db) { return db->vtable.abc(db); }

void db_openr(struct db *db, FILE *fp)
{
    cmp_setup(&db->file.cmp, fp);
    db->file.mode = DB_OPEN_READ;
}

enum rc db_openw(struct db *db, FILE *fp)
{
    enum rc rc = init_tmpdp(db);
    if (rc) return rc;

    cmp_setup(&db->file.cmp, fp);
    db->file.mode = DB_OPEN_WRITE;

    return init_tmpmeta(db);
}

static enum rc flush_metadata(struct db *db)
{
    struct cmp_ctx_s *cmp = &db->file.cmp;

    if (!cmp_write_u32(cmp, db->mt.size))
        return error(RC_EIO, "failed to write metadata size");

    char name[PROFILE_NAME_SIZE] = {0};
    char acc[PROFILE_ACC_SIZE] = {0};
    for (unsigned i = 0; i < db->profiles.size; ++i)
    {
        uint32_t len = ARRAY_SIZE(name) - 1;
        if (!cmp_read_cstr(&db->mt.file.cmp, name, &len))
            return error(RC_EIO, "failed to read name size");

        /* write the null-terminated character */
        if (!cmp_write(cmp, name, len + 1))
            return error(RC_EIO, "failed to write name");

        len = ARRAY_SIZE(acc) - 1;
        if (!cmp_read_cstr(&db->mt.file.cmp, acc, &len))
            return error(RC_EIO, "failed to read acc size");

        /* write the null-terminated character */
        if (!cmp_write(cmp, acc, len + 1))
            return error(RC_EIO, "failed to write acc");
    }

    return RC_DONE;
}

static void cleanup_metadata_data(struct db *db)
{
    free(db->mt.data);
    db->mt.data = NULL;
}

static void cleanup_metadata_parsing(struct db *db)
{
    free(db->mt.offset);
    free(db->mt.name_length);
    db->mt.offset = NULL;
    db->mt.name_length = NULL;
}

static void closer(struct db *db)
{
    assert(db->file.mode == DB_OPEN_READ);
    cleanup_metadata_data(db);
    cleanup_metadata_parsing(db);
}

static void cleanup(struct db *db)
{
    fclose(cmp_file(&db->mt.file.cmp));
    fclose(cmp_file(&db->dp.cmp));
}

static enum rc closew(struct db *db)
{
    assert(db->file.mode == DB_OPEN_WRITE);

    if (!cmp_write_u32(&db->file.cmp, db->profiles.size))
        return error(RC_EIO, "failed to write number of profiles");

    rewind(cmp_file(&db->mt.file.cmp));
    enum rc rc = RC_DONE;
    if ((rc = flush_metadata(db))) goto cleanup;
    if (fclose(cmp_file(&db->mt.file.cmp)))
    {
        rc = error(RC_EIO, "failed to close metadata file");
        goto cleanup;
    }

    rewind(cmp_file(&db->dp.cmp));
    if ((rc = xfile_copy(cmp_file(&db->file.cmp), cmp_file(&db->dp.cmp))))
        goto cleanup;

    if (fclose(cmp_file(&db->dp.cmp)))
    {
        rc = error(RC_EIO, "failed to close DP file");
        goto cleanup;
    }

    return rc;

cleanup:
    cleanup(db);
    return rc;
}

enum rc db_close(struct db *db)
{
    enum rc rc = db->vtable.close(db);
    if (rc) cleanup(db);
    return rc;
}

static inline uint32_t max_mt_data_size(void)
{
    return MAX_NPROFILES * (PROFILE_NAME_SIZE + PROFILE_ACC_SIZE);
}

enum rc db_read_magic_number(struct db *db)
{
    uint64_t magic_number = 0;
    if (!cmp_read_u64(&db->file.cmp, &magic_number))
        return error(RC_EIO, "failed to read magic number");

    if (magic_number != MAGIC_NUMBER)
        return error(RC_EPARSE, "wrong file magic number");

    return RC_DONE;
}

enum rc db_write_magic_number(struct db *db)
{
    if (!cmp_write_u64(&db->file.cmp, MAGIC_NUMBER))
        return error(RC_EIO, "failed to write magic number");

    return RC_DONE;
}

enum rc db_read_profile_typeid(struct db *db)
{
    uint8_t prof_typeid = 0;
    if (!cmp_read_u8(&db->file.cmp, &prof_typeid))
        return error(RC_EIO, "failed to read profile typeid");

    db->profile_typeid = (int)prof_typeid;
    return RC_DONE;
}

enum rc db_write_prof_type(struct db *db)
{
    if (!cmp_write_u8(&db->file.cmp, (uint8_t)db->vtable.typeid))
        return error(RC_EIO, "failed to write prof_type");

    return RC_DONE;
}

enum rc db_read_float_size(struct db *db)
{
    uint8_t float_bytes = 0;
    if (!cmp_read_u8(&db->file.cmp, &float_bytes))
        return error(RC_EIO, "failed to read float size");

    if (float_bytes != 4 && float_bytes != 8)
        return error(RC_EPARSE, "invalid float size");

    db->float_size = float_bytes;
    return RC_DONE;
}

enum rc db_write_float_size(struct db *db)
{
    unsigned size = IMM_FLOAT_BYTES;
    assert(size == 4 || size == 8);

    if (!cmp_write_u8(&db->file.cmp, (uint8_t)size))
        return error(RC_EIO, "failed to write float size");

    return RC_DONE;
}

enum rc db_read_nprofiles(struct db *db)
{
    if (!cmp_read_u32(&db->file.cmp, &db->profiles.size))
        return error(RC_EIO, "failed to read number of profiles");

    if (db->profiles.size > MAX_NPROFILES)
        return error(RC_EFAIL, "too many profiles");

    return RC_DONE;
}

static enum rc read_metadata_size(struct db *db)
{
    if (!cmp_read_u32(&db->file.cmp, &db->mt.size))
        return error(RC_EIO, "failed to read metadata size");

    if (db->mt.size > max_mt_data_size())
        return error(RC_EFAIL, "mt.data size is too big");

    return RC_DONE;
}

/* TODO: use it somewhere to make sure they are compatible */
static enum rc check_metadata_profile_compatibility(struct db const *db)
{
    if ((db->mt.size > 0 && db->profiles.size == 0) ||
        (db->mt.size == 0 && db->profiles.size > 0))
        return error(RC_EFAIL, "incompatible profiles and metadata");

    return RC_DONE;
}

static enum rc read_metadata_data(struct db *db)
{
    assert(db->mt.size > 0);

    if (!(db->mt.data = malloc(sizeof(char) * db->mt.size)))
        return error(RC_ENOMEM, "failed to alloc for mt.data");

    struct cmp_ctx_s *cmp = &db->file.cmp;
    if (!cmp_read(cmp, db->mt.data, db->mt.size))
    {
        cleanup_metadata_data(db);
        return error(RC_EIO, "failed to read metadata");
    }

    if (db->mt.data[db->mt.size - 1])
    {
        cleanup_metadata_data(db);
        return error(RC_EPARSE, "invalid metadata");
    }

    return RC_DONE;
}

static enum rc alloc_metadata_parsing(struct db *db)
{
    uint32_t n = db->profiles.size;

    if (!(db->mt.offset = malloc(sizeof(*db->mt.offset) * (n + 1))))
    {
        cleanup_metadata_parsing(db);
        return error(RC_ENOMEM, "failed to alloc for mt.offset");
    }

    if (!(db->mt.name_length = malloc(sizeof(*db->mt.name_length) * n)))
    {
        return error(RC_ENOMEM, "failed to alloc for mt.name_length");
        cleanup_metadata_parsing(db);
    }

    return RC_DONE;
}

static enum rc parse_metadata(struct db *db)
{
    assert(db->mt.offset);
    assert(db->mt.name_length);

    db->mt.offset[0] = 0;
    for (unsigned i = 0; i < db->profiles.size; ++i)
    {
        unsigned offset = db->mt.offset[i];
        unsigned j = 0;
        if (offset + j >= db->mt.size)
            return error(RC_EFAIL, "mt.data index overflow");

        /* Name */
        while (db->mt.data[offset + j++])
            ;
        if (j > PROFILE_NAME_SIZE) return error(RC_EINVAL, "name is too long");

        db->mt.name_length[i] = (uint8_t)(j - 1);
        if (offset + j >= db->mt.size)
            return error(RC_EFAIL, "mt.data index overflow");

        /* Accession */
        while (db->mt.data[offset + j++])
            ;
        db->mt.offset[i + 1] = offset + j;
    }

    return RC_DONE;
}

enum rc db_read_metadata(struct db *db)
{
    enum rc rc = RC_DONE;

    if ((rc = read_metadata_size(db))) goto cleanup;

    if (db->mt.size > 0)
    {
        if ((rc = read_metadata_data(db))) goto cleanup;
        if ((rc = alloc_metadata_parsing(db))) goto cleanup;
        if ((rc = parse_metadata(db))) goto cleanup;
    }

    return RC_DONE;

cleanup:
    cleanup_metadata_data(db);
    cleanup_metadata_parsing(db);
    return rc;
}

static enum rc write_name(struct db *db, struct metadata mt)
{
    struct cmp_ctx_s *ctx = &db->mt.file.cmp;

    if (!cmp_write_str(ctx, mt.name, (uint32_t)strlen(mt.name)))
        return error(RC_EIO, "failed to write profile name");
    /* +1 for null-terminated */
    db->mt.size += (uint32_t)strlen(mt.name) + 1;

    return RC_DONE;
}

static enum rc write_accession(struct db *db, struct metadata mt)
{
    struct cmp_ctx_s *ctx = &db->mt.file.cmp;

    if (!cmp_write_str(ctx, mt.acc, (uint32_t)strlen(mt.acc)))
        return error(RC_EIO, "failed to write profile accession");
    /* +1 for null-terminated */
    db->mt.size += (uint32_t)strlen(mt.acc) + 1;

    return RC_DONE;
}

enum rc db_write_profile_metadata(struct db *db, struct metadata mt)
{
    if (mt.name == NULL) return error(RC_EINVAL, "metadata not set");

    if (strlen(mt.name) >= PROFILE_NAME_SIZE)
        return error(RC_EINVAL, "profile name is too long");

    if (strlen(mt.acc) >= PROFILE_ACC_SIZE)
        return error(RC_EINVAL, "profile accession is too long");

    enum rc rc = RC_DONE;

    if ((rc = write_name(db, mt))) return rc;
    if ((rc = write_accession(db, mt))) return rc;

    return RC_DONE;
}

struct metadata db_metadata(struct db const *db, unsigned idx)
{
    unsigned o = db->mt.offset[idx];
    unsigned size = (unsigned)(db->mt.name_length[idx] + 1);
    return metadata(db->mt.data + o, db->mt.data + o + size);
}

unsigned db_float_size(struct db const *db) { return db->float_size; }

int db_profile_typeid(struct db const *db) { return db->profile_typeid; }

int db_typeid(struct db const *db) { return db->vtable.typeid; }

off_t db_profiles_block_offset(struct db const *db)
{
    return db->profiles_block_offset;
}

enum rc db_set_metadata_end(struct db *db)
{
    FILE *fp = cmp_file(&db->file.cmp);
    if ((db->profiles_block_offset = ftello(fp)) == -1)
        return error(RC_EIO, "failed to ftello");
    return RC_DONE;
}

enum rc __db_close(struct db *db)
{
    if (db->file.mode == DB_OPEN_WRITE) return closew(db);
    closer(db);
    return RC_DONE;
}

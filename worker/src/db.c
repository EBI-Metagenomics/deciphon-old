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

static enum rc copy_tmp_prof(struct cmp_ctx_s *dst, struct cmp_ctx_s *tmp,
                             unsigned nprofiles)
{
    rewind(cmp_file(tmp));
    for (unsigned i = 0; i <= nprofiles; ++i)
    {
        int64_t offset = 0;
        if (!cmp_read_s64(tmp, &offset)) return eio("read offset");
        if (!cmp_write_s64(dst, offset)) return eio("write offset");
    }

    return RC_DONE;
}

static enum rc copy_tmp_mt(struct cmp_ctx_s *dst, struct cmp_ctx_s *tmp,
                           unsigned nprofiles)
{
    rewind(cmp_file(tmp));

    char name[PROFILE_NAME_SIZE] = {0};
    char acc[PROFILE_ACC_SIZE] = {0};
    for (unsigned i = 0; i < nprofiles; ++i)
    {
        uint32_t len = ARRAY_SIZE(name) - 1;
        if (!cmp_read_cstr(tmp, name, &len)) return eio("read name size");

        /* write the null-terminated character */
        if (!cmp_write(dst, name, len + 1)) return eio("write name");

        len = ARRAY_SIZE(acc) - 1;
        if (!cmp_read_cstr(tmp, acc, &len)) return eio("read acc size");

        /* write the null-terminated character */
        if (!cmp_write(dst, acc, len + 1)) return eio("write acc");
    }

    return RC_DONE;
}

void db_init(struct db *db, struct db_vtable vtable)
{
    db->vtable = vtable;
    db->float_size = IMM_FLOAT_BYTES;
    db->profile_offsets = 0;
    db->profiles_block_offset = 0;
    db->nprofiles = 0;
    db_mt_init(&db->mt);
    db_tmp_setup(&db->tmp);
    cmp_setup(&db->file.cmp, 0);
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
    cmp_setup(&db->file.cmp, fp);
    db->file.mode = DB_OPEN_WRITE;
    return db_tmp_init(&db->tmp);
}

static void closer(struct db *db)
{
    db_mt_cleanup(&db->mt);
    free(db->profile_offsets);
    assert(db->file.mode == DB_OPEN_READ);
}

static enum rc write_profile_offset(struct db *db, int64_t offset)
{
    if (!cmp_write_s64(&db->tmp.prof_cmp, offset))
        return eio("write profile offset");

    return RC_DONE;
}

static enum rc closew(struct db *db)
{
    assert(db->file.mode == DB_OPEN_WRITE);
    int64_t offset = cmp_ftell(&db->tmp.dp_cmp);
    if (offset < 0) eio("ftell");

    enum rc rc = write_profile_offset(db, offset);
    if (rc) return rc;

    if (!cmp_write_u32(&db->file.cmp, (uint32_t)db->nprofiles))
        return eio("write number of profiles");

    if ((rc = copy_tmp_prof(&db->file.cmp, &db->tmp.prof_cmp, db->nprofiles)))
        goto cleanup;

    if (!cmp_write_u32(&db->file.cmp, db->mt.size))
        return eio("write metadata size");

    if ((rc = copy_tmp_mt(&db->file.cmp, &db->tmp.mt_cmp, db->nprofiles)))
        goto cleanup;

    rewind(cmp_file(&db->tmp.dp_cmp));
    if ((rc = xfile_copy(cmp_file(&db->file.cmp), cmp_file(&db->tmp.dp_cmp))))
        goto cleanup;

cleanup:
    db_tmp_close(&db->tmp);
    return rc;
}

enum rc db_close(struct db *db)
{
    if (db->file.mode == DB_OPEN_WRITE) return closew(db);
    closer(db);
    return RC_DONE;
}

void db_cleanup(struct db *db)
{
    db_mt_cleanup(&db->mt);
    db_tmp_close(&db->tmp);
}

enum rc db_read_magic_number(struct db *db)
{
    uint64_t magic_number = 0;
    if (!cmp_read_u64(&db->file.cmp, &magic_number))
        return eio("read magic number");

    if (magic_number != MAGIC_NUMBER)
        return error(RC_EPARSE, "wrong file magic number");

    return RC_DONE;
}

enum rc db_write_magic_number(struct db *db)
{
    if (!cmp_write_u64(&db->file.cmp, MAGIC_NUMBER))
        return eio("write magic number");

    return RC_DONE;
}

enum rc db_read_profile_typeid(struct db *db)
{
    uint8_t prof_typeid = 0;
    if (!cmp_read_u8(&db->file.cmp, &prof_typeid))
        return eio("read profile typeid");

    db->profile_typeid = (int)prof_typeid;
    return RC_DONE;
}

enum rc db_write_prof_type(struct db *db)
{
    if (!cmp_write_u8(&db->file.cmp, (uint8_t)db->vtable.typeid))
        return eio("write prof_type");

    return RC_DONE;
}

enum rc db_read_float_size(struct db *db)
{
    uint8_t float_bytes = 0;
    if (!cmp_read_u8(&db->file.cmp, &float_bytes))
        return eio("read float size");

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
        return eio("write float size");

    return RC_DONE;
}

enum rc db_read_nprofiles(struct db *db)
{
    uint32_t nprofiles = (uint32_t)db->nprofiles;
    if (!cmp_read_u32(&db->file.cmp, &nprofiles))
        return eio("read number of profiles");

    if (db->nprofiles > MAX_NPROFILES)
        return error(RC_EFAIL, "too many profiles");

    return RC_DONE;
}

/* TODO: use it somewhere to make sure they are compatible */
static enum rc check_metadata_profile_compatibility(struct db const *db)
{
    if ((db->mt.size > 0 && db->nprofiles == 0) ||
        (db->mt.size == 0 && db->nprofiles > 0))
        return error(RC_EFAIL, "incompatible profiles and metadata");

    return RC_DONE;
}

enum rc db_read_profile_offsets(struct db *db)
{
    size_t sz = sizeof(*db->profile_offsets) * (db->nprofiles + 1);
    db->profile_offsets = malloc(sz);

    if (!db->profile_offsets) return error(RC_ENOMEM, "no memory for offsets");

    for (unsigned i = 0; i <= db->nprofiles; ++i)
    {
        if (!cmp_read_s64(&db->file.cmp, db->profile_offsets + i))
        {
            enum rc rc = eio("read profile offset");
            free(db->profile_offsets);
            db->profile_offsets = 0;
            return rc;
        }
    }
    return RC_DONE;
}

enum rc db_read_metadata(struct db *db)
{
    return db_mt_read(&db->mt, db->nprofiles, &db->file.cmp);
}

enum rc db_write_profile(struct db *db, struct profile const *prof,
                         struct metadata mt)
{
    int64_t offset = cmp_ftell(&db->tmp.dp_cmp);
    if (offset < 0) eio("ftell");

    enum rc rc = write_profile_offset(db, offset);
    if (rc) return rc;

    rc = db_mt_write(&db->mt, mt, &db->tmp.mt_cmp);
    if (rc) return rc;

    rc = db->vtable.write_profile(&db->tmp.dp_cmp, prof, mt);
    if (rc) return rc;

    db->nprofiles++;
    return rc;
}

struct metadata db_metadata(struct db const *db, unsigned idx)
{
    return db_mt_metadata(&db->mt, idx);
}

unsigned db_float_size(struct db const *db) { return db->float_size; }

int db_profile_typeid(struct db const *db) { return db->profile_typeid; }

int db_typeid(struct db const *db) { return db->vtable.typeid; }

int64_t db_profiles_block_offset(struct db const *db)
{
    return db->profiles_block_offset;
}

enum rc db_set_metadata_end(struct db *db)
{
    if ((db->profiles_block_offset = cmp_ftell(&db->file.cmp)) == -1)
        return eio("ftello");
    return RC_DONE;
}

#include "db.h"
#include "common/compiler.h"
#include "common/limits.h"
#include "common/logger.h"
#include "common/rc.h"
#include "common/xfile.h"
#include "imm/imm.h"
#include "js.h"
#include "profile.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* TODO: make sure there is no accession duplicates. */

#define MAGIC_NUMBER 0xC6F0

static enum rc copy_tmp_prof(struct lip_io_file *dst, struct lip_io_file *tmp,
                             unsigned nprofiles, int64_t base_offset)
{
    rewind(cmp_file(tmp));
    for (unsigned i = 0; i <= nprofiles; ++i)
    {
        int64_t offset = 0;
        if (!cmp_read_s64(tmp, &offset)) return eio("read offset");
        if (!cmp_write_s64(dst, base_offset + offset))
            return eio("write offset");
    }

    return RC_DONE;
}

static enum rc copy_tmp_mt(struct lip_io_file *dst, struct lip_io_file *tmp,
                           unsigned nprofiles)
{
    rewind(cmp_file(tmp));

    char name[PROFILE_NAME_SIZE] = {0};
    char acc[PROFILE_ACC_SIZE] = {0};
    for (unsigned i = 0; i < nprofiles; ++i)
    {
        uint32_t len = ARRAY_SIZE(name) - 1;
        if (!js_read_str(tmp, name, &len)) return eio("read name size");

        /* write the null-terminated character */
        if (!cmp_write(dst, name, len + 1)) return eio("write name");

        len = ARRAY_SIZE(acc) - 1;
        if (!js_read_str(tmp, acc, &len)) return eio("read acc size");

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
    uint32_t size = 0;
    cmp_read_map(&db->file.cmp, &size);
    assert(size == 3);
    db->file.mode = DB_OPEN_READ;
}

static enum rc write_header_root_map(struct lip_io_file *cmp, unsigned size)
{
    if (!cmp_write_map(cmp, size)) return eio("write header map size");
    return RC_DONE;
}

enum rc db_openw(struct db *db, FILE *fp)
{
    cmp_setup(&db->file.cmp, fp);
    db->file.mode = DB_OPEN_WRITE;

    enum rc rc = db_tmp_init(&db->tmp);
    if (rc) goto cleanup;

    rc = write_header_root_map(&db->tmp.hdr, db->vtable.header_size);
    if (rc) goto cleanup;

    return RC_DONE;

cleanup:
    db_tmp_close(&db->tmp);
    return rc;
}

static void closer(struct db *db)
{
    db_mt_cleanup(&db->mt);
    free(db->profile_offsets);
    assert(db->file.mode == DB_OPEN_READ);
}

static enum rc write_nprofiles(struct db *db)
{
    unsigned nprofs = db->nprofiles;
    if (!cmp_write_u32(&db->file.cmp, (uint32_t)nprofs))
        return eio("write number of profiles");
    return RC_DONE;
}

static enum rc closew(struct db *db)
{
    assert(db->file.mode == DB_OPEN_WRITE);

    enum rc rc = RC_DONE;
    if (!cmp_write_map(&db->file.cmp, 3))
    {
        rc = eio("write root map size");
        goto cleanup;
    }

    if (!JS_WRITE_STR(&db->tmp.hdr, "profile_size"))
    {
        rc = eio("write profile_size key");
        goto cleanup;
    }
    if (!cmp_write_array(&db->tmp.hdr, db->nprofiles))
    {
        rc = eio("write array size");
        goto cleanup;
    }
    rewind(cmp_file(&db->tmp.size));
    if ((rc = xfile_copy(cmp_file(&db->tmp.hdr), cmp_file(&db->tmp.size))))
        goto cleanup;

    rewind(cmp_file(&db->tmp.hdr));
    if (!JS_WRITE_STR(&db->file.cmp, "header"))
    {
        rc = eio("write header key");
        goto cleanup;
    }
    if ((rc = xfile_copy(cmp_file(&db->file.cmp), cmp_file(&db->tmp.hdr))))
        goto cleanup;

    if (!JS_WRITE_STR(&db->file.cmp, "metadata"))
    {
        rc = eio("write metadata key");
        goto cleanup;
    }
    int64_t fs = 0;
    xfile_psize(cmp_file(&db->tmp.mt), &fs);
    rewind(cmp_file(&db->tmp.mt));
    if (!cmp_write_bin_marker(&db->file.cmp, (uint32_t)fs))
    {
        rc = eio("write array size");
        goto cleanup;
    }
    if ((rc = copy_tmp_mt(&db->file.cmp, &db->tmp.mt, db->nprofiles)))
        goto cleanup;

    if (!JS_WRITE_STR(&db->file.cmp, "profile"))
    {
        rc = eio("write profile key");
        goto cleanup;
    }
    rewind(cmp_file(&db->tmp.prof));
    if ((rc = xfile_copy(cmp_file(&db->file.cmp), cmp_file(&db->tmp.prof))))
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

static enum rc db_write_header_key(struct lip_io_file *io, unsigned size)
{
    if (!cmp_write_map(io, size)) eio("write header map size");
    if (!JS_WRITE_STR(io, "header")) eio("write header map key");
    return RC_DONE;
}

static enum rc db_write_profile_key(struct lip_io_file *io, unsigned size)
{
    if (!cmp_write_map(io, size)) eio("write profile map size");
    if (!JS_WRITE_STR(io, "profile")) eio("write profile map key");
    return RC_DONE;
}

enum rc db_read_magic_number(struct db *db)
{
    struct lip_io_file *io = &db->file.cmp;
    if (!JS_XPEC_STR(io, "magic_number")) return eio("skip magic_number key");

    int64_t magic_number = 0;
    if (!cmp_read_integer(io, &magic_number)) return eio("read magic number");

    if (magic_number != MAGIC_NUMBER)
        return error(RC_EPARSE, "wrong file magic number");

    return RC_DONE;
}

enum rc db_write_magic_number(struct db *db)
{
    struct lip_io_file *io = &db->tmp.hdr;
    if (!JS_WRITE_STR(io, "magic_number")) eio("write magic_number map key");
    if (!cmp_write_integer(io, MAGIC_NUMBER)) eio("write magic number");
    return RC_DONE;
}

enum rc db_read_profile_typeid(struct db *db)
{
    struct lip_io_file *io = &db->file.cmp;
    if (!JS_XPEC_STR(io, "profile_typeid"))
        return eio("skip profile_typeid key");
    int64_t v = 0;
    if (!cmp_read_integer(io, &v)) return eio("read profile typeid");
    db->profile_typeid = (int)v;
    return RC_DONE;
}

enum rc db_write_profile_typeid(struct db *db)
{
    struct lip_io_file *io = &db->tmp.hdr;
    if (!JS_WRITE_STR(io, "profile_typeid"))
        eio("write profile_typeid map key");
    if (!cmp_write_integer(io, db->profile_typeid))
        return eio("write profile_typeid");
    return RC_DONE;
}

enum rc db_read_float_size(struct db *db)
{
    struct lip_io_file *io = &db->file.cmp;
    if (!JS_XPEC_STR(io, "float_size")) return eio("skip float_size key");

    uint64_t v = 0;
    if (!cmp_read_uinteger(io, &v)) return eio("read float size");
    if (v != 4 && v != 8) return error(RC_EPARSE, "invalid float size");

    db->float_size = (unsigned)v;
    return RC_DONE;
}

enum rc db_write_float_size(struct db *db)
{
    struct lip_io_file *io = &db->tmp.hdr;
    if (!JS_WRITE_STR(io, "float_size")) eio("write float_size map key");

    unsigned size = IMM_FLOAT_BYTES;
    assert(size == 4 || size == 8);
    if (!cmp_write_uinteger(io, size)) return eio("write float size");
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

enum rc db_read_profile_sizes(struct db *db)
{
    struct lip_io_file *io = &db->file.cmp;
    if (!JS_XPEC_STR(io, "profile_size")) eio("skip profile_size key");

    uint32_t n = 0;
    if (!cmp_read_array(io, &n)) return eio("read array size");
    assert(n <= MAX_NPROFILES);
    db->nprofiles = (unsigned)n;

    size_t sz = sizeof(*db->profile_offsets) * (db->nprofiles + 1);
    db->profile_offsets = malloc(sz);

    if (!db->profile_offsets) return error(RC_ENOMEM, "no memory for offsets");

    db->profile_offsets[0] = 0;
    for (unsigned i = 0; i < db->nprofiles; ++i)
    {
        uint64_t size = 0;
        if (!cmp_read_uinteger(&db->file.cmp, &size))
        {
            enum rc rc = eio("read profile size");
            free(db->profile_offsets);
            db->profile_offsets = 0;
            return rc;
        }
        db->profile_offsets[i + 1] = db->profile_offsets[i] + (int64_t)size;
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
    enum rc rc = db_mt_write(&db->mt, mt, &db->tmp.mt);
    if (rc) return rc;

    int64_t start = cmp_ftell(&db->tmp.prof);
    if (start < 0) eio("ftell");

    rc = db->vtable.write_profile(&db->tmp.prof, prof);
    if (rc) return rc;

    int64_t end = cmp_ftell(&db->tmp.prof);
    if (end < 0) eio("ftell");

    uint64_t prof_size = (uint64_t)(end - start);
    if (!cmp_write_uinteger(&db->tmp.size, prof_size))
        return eio("write profile size");

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

enum rc db_set_metadata_end(struct db *db)
{
    if (!JS_XPEC_STR(&db->file.cmp, "profile")) return eio("skip key");
    int64_t off = cmp_ftell(&db->file.cmp);
    for (unsigned i = 0; i <= db->nprofiles; ++i)
        db->profile_offsets[i] += off;
    return RC_DONE;
}

#include "db.h"
#include "dcp/compiler.h"
#include "dcp/rc.h"
#include "expect.h"
#include "imm/imm.h"
#include "limits.h"
#include "logger.h"
#include "profile.h"
#include "xfile.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* TODO: make sure there is no accession duplicates. */

#define MAGIC_NUMBER 0xC6F0

static enum rc copy_tmp_prof(struct lip_file *dst, struct lip_file *tmp,
                             unsigned nprofiles, int64_t base_offset)
{
#if 0
    rewind(lip_file(tmp));
    for (unsigned i = 0; i <= nprofiles; ++i)
    {
        int64_t offset = 0;
        if (!lip_read_s64(tmp, &offset)) return eio("read offset");
        if (!lip_write_s64(dst, base_offset + offset))
            return eio("write offset");
    }

#endif
    return DCP_OK;
}

static enum rc copy_tmp_mt(struct lip_file *dst, struct lip_file *tmp,
                           unsigned nprofiles)
{
#if 0
    rewind(lip_file(tmp));

    char name[PROFILE_NAME_SIZE] = {0};
    char acc[PROFILE_ACC_SIZE] = {0};
    for (unsigned i = 0; i < nprofiles; ++i)
    {
        uint32_t len = ARRAY_SIZE(name) - 1;
        if (!js_read_str(tmp, name, &len)) return eio("read name size");

        /* write the null-terminated character */
        if (!lip_write(dst, name, len + 1)) return eio("write name");

        len = ARRAY_SIZE(acc) - 1;
        if (!js_read_str(tmp, acc, &len)) return eio("read acc size");

        /* write the null-terminated character */
        if (!lip_write(dst, acc, len + 1)) return eio("write acc");
    }

#endif
    return DCP_OK;
}

void db_init(struct db *db, struct db_vtable vtable)
{
    db->vtable = vtable;
    db->float_size = IMM_FLOAT_BYTES;
    db->profile_offsets = 0;
    db->nprofiles = 0;
    db_mt_init(&db->mt);
    db_tmp_setup(&db->tmp);
    lip_file_init(&db->file.lip);
    db->file.mode = DB_OPEN_NULL;
}

struct imm_abc const *db_abc(struct db const *db) { return db->vtable.abc(db); }

void db_openr(struct db *db, FILE *fp)
{
    lip_file_init(&db->file.lip);
    db->file.lip.fp = fp;
    unsigned size = 0;
    lip_read_map_size(&db->file.lip, &size);
    assert(size == 3);
    db->file.mode = DB_OPEN_READ;
}

static enum rc write_header_root_map(struct lip_file *cmp, unsigned size)
{
    if (!lip_write_map_size(cmp, size)) return eio("write header map size");
    return DCP_OK;
}

enum rc db_openw(struct db *db, FILE *fp)
{
    lip_file_init(&db->file.lip);
    db->file.lip.fp = fp;
    db->file.mode = DB_OPEN_WRITE;

    enum rc rc = db_tmp_init(&db->tmp);
    if (rc) goto cleanup;

    rc = write_header_root_map(&db->tmp.hdr, db->vtable.header_size);
    if (rc) goto cleanup;

    return DCP_OK;

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
    if (!lip_write_int(&db->file.lip, nprofs))
        return eio("write number of profiles");
    return DCP_OK;
}

static enum rc closew(struct db *db)
{
    assert(db->file.mode == DB_OPEN_WRITE);

    enum rc rc = DCP_OK;
    if (!lip_write_map_size(&db->file.lip, 3))
    {
        rc = eio("write root map size");
        goto cleanup;
    }

    if (!lip_write_cstr(&db->tmp.hdr, "profile_size"))
    {
        rc = eio("write profile_size key");
        goto cleanup;
    }
    if (!lip_write_array_size(&db->tmp.hdr, db->nprofiles))
    {
        rc = eio("write array size");
        goto cleanup;
    }
    lip_rewind(&db->tmp.size);
    if ((rc = xfile_copy(db->tmp.hdr.fp, db->tmp.size.fp))) goto cleanup;

    lip_rewind(&db->tmp.hdr);
    if (!lip_write_cstr(&db->file.lip, "header"))
    {
        rc = eio("write header key");
        goto cleanup;
    }
    if ((rc = xfile_copy(db->file.lip.fp, db->tmp.hdr.fp))) goto cleanup;

    if (!lip_write_cstr(&db->file.lip, "metadata"))
    {
        rc = eio("write metadata key");
        goto cleanup;
    }
    int64_t fs = 0;
    xfile_psize(db->tmp.mt.fp, &fs);
    lip_rewind(&db->tmp.mt);
    // TODO: use 1darray
    // if (!lip_write_bin_marker(&db->file.lip, (uint32_t)fs))
    // {
    //     rc = eio("write array size");
    //     goto cleanup;
    // }
    if ((rc = copy_tmp_mt(&db->file.lip, &db->tmp.mt, db->nprofiles)))
        goto cleanup;

    if (!lip_write_cstr(&db->file.lip, "profile"))
    {
        rc = eio("write profile key");
        goto cleanup;
    }
    lip_rewind(&db->tmp.prof);
    if ((rc = xfile_copy(db->file.lip.fp, db->tmp.prof.fp))) goto cleanup;

cleanup:
    db_tmp_close(&db->tmp);
    return rc;
}

enum rc db_close(struct db *db)
{
    if (db->file.mode == DB_OPEN_WRITE) return closew(db);
    closer(db);
    return DCP_OK;
}

void db_cleanup(struct db *db)
{
    db_mt_cleanup(&db->mt);
    db_tmp_close(&db->tmp);
}

static enum rc db_write_header_key(struct lip_file *io, unsigned size)
{
    if (!lip_write_map_size(io, size)) eio("write header map size");
    if (!lip_write_cstr(io, "header")) eio("write header map key");
    return DCP_OK;
}

static enum rc db_write_profile_key(struct lip_file *io, unsigned size)
{
    if (!lip_write_map_size(io, size)) eio("write profile map size");
    if (!lip_write_cstr(io, "profile")) eio("write profile map key");
    return DCP_OK;
}

enum rc db_read_magic_number(struct db *db)
{
    struct lip_file *io = &db->file.lip;
    if (!expect_key(io, "magic_number")) return eio("skip magic_number key");

    int magic_number = 0;
    if (!lip_read_int(io, &magic_number)) return eio("read magic number");

    if (magic_number != MAGIC_NUMBER)
        return error(DCP_EPARSE, "wrong file magic number");

    return DCP_OK;
}

enum rc db_write_magic_number(struct db *db)
{
    struct lip_file *io = &db->tmp.hdr;
    if (!lip_write_cstr(io, "magic_number")) eio("write magic_number map key");
    if (!lip_write_int(io, MAGIC_NUMBER)) eio("write magic number");
    return DCP_OK;
}

enum rc db_read_profile_typeid(struct db *db)
{
    struct lip_file *io = &db->file.lip;
    if (!expect_key(io, "profile_typeid"))
        return eio("skip profile_typeid key");
    int v = 0;
    if (!lip_read_int(io, &v)) return eio("read profile typeid");
    db->profile_typeid = v;
    return DCP_OK;
}

enum rc db_write_profile_typeid(struct db *db)
{
    struct lip_file *io = &db->tmp.hdr;
    if (!lip_write_cstr(io, "profile_typeid"))
        eio("write profile_typeid map key");
    if (!lip_write_int(io, db->profile_typeid))
        return eio("write profile_typeid");
    return DCP_OK;
}

enum rc db_read_float_size(struct db *db)
{
    struct lip_file *io = &db->file.lip;
    if (!expect_key(io, "float_size")) return eio("skip float_size key");

    unsigned size = 0;
    if (!lip_read_int(io, &size)) return eio("read float size");
    if (size != 4 && size != 8) return error(DCP_EPARSE, "invalid float size");

    db->float_size = (unsigned)size;
    return DCP_OK;
}

enum rc db_write_float_size(struct db *db)
{
    struct lip_file *io = &db->tmp.hdr;
    if (!lip_write_cstr(io, "float_size")) eio("write float_size map key");

    unsigned size = IMM_FLOAT_BYTES;
    assert(size == 4 || size == 8);
    if (!lip_write_int(io, size)) return eio("write float size");
    return DCP_OK;
}

/* TODO: use it somewhere to make sure they are compatible */
static enum rc check_metadata_profile_compatibility(struct db const *db)
{
    if ((db->mt.size > 0 && db->nprofiles == 0) ||
        (db->mt.size == 0 && db->nprofiles > 0))
        return error(DCP_EFAIL, "incompatible profiles and metadata");

    return DCP_OK;
}

enum rc db_read_profile_sizes(struct db *db)
{
    struct lip_file *io = &db->file.lip;
    if (!expect_key(io, "profile_size")) eio("skip profile_size key");

    uint32_t n = 0;
    // TODO: use 1darray
    if (!lip_read_array_size(io, &n)) return eio("read array size");
    assert(n <= MAX_NPROFILES);
    db->nprofiles = (unsigned)n;

    size_t sz = sizeof(*db->profile_offsets) * (db->nprofiles + 1);
    db->profile_offsets = malloc(sz);

    if (!db->profile_offsets) return error(DCP_ENOMEM, "no memory for offsets");

    db->profile_offsets[0] = 0;
    for (unsigned i = 0; i < db->nprofiles; ++i)
    {
        unsigned size = 0;
        if (!lip_read_int(&db->file.lip, &size))
        {
            enum rc rc = eio("read profile size");
            free(db->profile_offsets);
            db->profile_offsets = 0;
            return rc;
        }
        db->profile_offsets[i + 1] = db->profile_offsets[i] + (int64_t)size;
    }
    return DCP_OK;
}

enum rc db_read_metadata(struct db *db)
{
    return db_mt_read(&db->mt, db->nprofiles, &db->file.lip);
}

enum rc db_write_profile(struct db *db, struct profile const *prof,
                         struct metadata mt)
{
    enum rc rc = db_mt_write(&db->mt, mt, &db->tmp.mt);
    if (rc) return rc;

    int64_t start = lip_ftell(&db->tmp.prof);
    if (start < 0) eio("ftell");

    rc = db->vtable.write_profile(&db->tmp.prof, prof);
    if (rc) return rc;

    int64_t end = lip_ftell(&db->tmp.prof);
    if (end < 0) eio("ftell");

    assert((end - start) <= UINT_MAX);
    unsigned prof_size = (unsigned)(end - start);
    if (!lip_write_int(&db->tmp.size, prof_size))
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
    if (!expect_key(&db->file.lip, "profile")) return eio("skip key");
    int64_t off = lip_ftell(&db->file.lip);
    for (unsigned i = 0; i <= db->nprofiles; ++i)
        db->profile_offsets[i] += off;
    return DCP_OK;
}

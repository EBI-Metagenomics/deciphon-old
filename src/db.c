#include "db.h"
#include "dcp.h"
#include "dcp/generics.h"
#include "dcp/profile.h"
#include "error.h"
#include "fcopy.h"
#include "imm/imm.h"
#include "profile.h"
#include "xcmp.h"
#include <assert.h>

#define MAGIC_NUMBER 0x765C806BF0E8652B

#define MAX_NPROFILES (1U << 20)

#define MAX_NAME_SIZE 63
#define MAX_ACC_SIZE 31

static void init_db_file(struct dcp_db *db, FILE *restrict fd,
                         enum db_mode mode)
{
    db->file.fd = fd;
    xcmp_init(&db->file.ctx, db->file.fd);
    db->file.mode = mode;
}

static enum dcp_rc init_meta_file(struct dcp_db *db, FILE *restrict fd)
{
    if (!(db->mt.file.fd = tmpfile()))
        return error(DCP_IOERROR, "tmpfile() failed");

    xcmp_init(&db->mt.file.ctx, db->mt.file.fd);
    return DCP_SUCCESS;
}

void db_init(struct dcp_db *db, enum dcp_prof_typeid prof_typeid)
{
    db->prof_typeid = prof_typeid;
    db->float_bytes = IMM_FLOAT_BYTES;
    db->profiles.size = 0;
    db->profiles.curr_idx = 0;
    db->mt.offset = NULL;
    db->mt.name_length = NULL;
    db->mt.data = NULL;
    db->mt.file.fd = NULL;
    db->dp.fd = NULL;
    db->file.fd = NULL;
}

void db_openr(struct dcp_db *db, FILE *restrict fd)
{
    init_db_file(db, fd, DB_OPEN_READ);
}

enum dcp_rc db_openw(struct dcp_db *db, FILE *restrict fd)
{
    if (!(db->dp.fd = tmpfile())) return error(DCP_IOERROR, "tmpfile() failed");
    init_db_file(db, fd, DB_OPEN_WRITE);
    return init_meta_file(db, fd);
}

static enum dcp_rc flush_metadata(struct dcp_db *db)
{
    cmp_ctx_t *ctx = &db->file.ctx;

    if (!cmp_write_u32(ctx, db->mt.size))
        return error(DCP_IOERROR, "failed to write metadata size");

    char name[MAX_NAME_SIZE + 1] = {0};
    char acc[MAX_ACC_SIZE + 1] = {0};
    for (unsigned i = 0; i < db->profiles.size; ++i)
    {
        uint32_t size = ARRAY_SIZE(name);
        if (!cmp_read_str(&db->mt.file.ctx, name, &size))
            return error(DCP_IOERROR, "failed to read name size");

        if (!ctx->write(ctx, name, size + 1))
            return error(DCP_IOERROR, "failed to write name");

        size = ARRAY_SIZE(acc);
        if (!cmp_read_str(&db->mt.file.ctx, acc, &size))
            return error(DCP_IOERROR, "failed to read acc size");

        if (!ctx->write(ctx, acc, size + 1))
            return error(DCP_IOERROR, "failed to write acc");
    }

    return DCP_SUCCESS;
}

static void closer(struct dcp_db const *db)
{
    assert(db->file.mode == DB_OPEN_READ);
}

static enum dcp_rc closew(struct dcp_db *db)
{
    assert(db->file.mode == DB_OPEN_WRITE);

    enum dcp_rc rc = DCP_SUCCESS;

    if (!cmp_write_u32(&db->file.ctx, db->profiles.size))
        return error(DCP_IOERROR, "failed to write number of profiles");

    rewind(db->mt.file.fd);
    if ((rc = flush_metadata(db))) goto cleanup;
    if (fclose(db->mt.file.fd))
    {
        rc = error(DCP_IOERROR, "failed to close metadata file");
        goto cleanup;
    }

    rewind(db->dp.fd);
    if ((rc = fcopy(db->file.fd, db->dp.fd))) goto cleanup;
    if (fclose(db->dp.fd))
    {
        rc = error(DCP_IOERROR, "failed to close DP file");
        goto cleanup;
    }

    return rc;

cleanup:
    fclose(db->mt.file.fd);
    fclose(db->dp.fd);
    return rc;
}

enum dcp_rc db_close(struct dcp_db *db)
{
    if (db->file.mode == DB_OPEN_WRITE) return closew(db);
    closer(db);
    return DCP_SUCCESS;
}

bool db_end(struct dcp_db const *db)
{
    return db->profiles.curr_idx >= db->profiles.size;
}

static inline uint32_t max_mt_data_size(void)
{
    return MAX_NPROFILES * (MAX_NAME_SIZE + MAX_ACC_SIZE + 2);
}

enum dcp_rc db_read_magic_number(struct dcp_db *db)
{
    uint64_t magic_number = 0;
    if (!cmp_read_u64(&db->file.ctx, &magic_number))
        return error(DCP_IOERROR, "failed to read magic number");

    if (magic_number != MAGIC_NUMBER)
        return error(DCP_PARSEERROR, "wrong file magic number");

    return DCP_SUCCESS;
}

enum dcp_rc db_write_magic_number(struct dcp_db *db)
{
    if (!cmp_write_u64(&db->file.ctx, MAGIC_NUMBER))
        return error(DCP_IOERROR, "failed to write magic number");

    return DCP_SUCCESS;
}

enum dcp_rc db_read_prof_type(struct dcp_db *db)
{
    uint8_t prof_type = 0;
    if (!cmp_read_u8(&db->file.ctx, &prof_type))
        return error(DCP_IOERROR, "failed to read profile type");

    if (prof_type != DCP_STD_PROFILE && prof_type != DCP_PROTEIN_PROFILE)
        return error(DCP_PARSEERROR, "wrong prof_type");

    db->prof_typeid = prof_type;
    return DCP_SUCCESS;
}

enum dcp_rc db_write_prof_type(struct dcp_db *db)
{
    if (!cmp_write_u8(&db->file.ctx, (uint8_t)db->prof_typeid))
        return error(DCP_IOERROR, "failed to write prof_type");

    return DCP_SUCCESS;
}

enum dcp_rc db_read_float_bytes(struct dcp_db *db)
{
    uint8_t float_bytes = 0;
    if (!cmp_read_u8(&db->file.ctx, &float_bytes))
        return error(DCP_IOERROR, "failed to read float size");

    if (float_bytes != 4 && float_bytes != 8)
        return error(DCP_PARSEERROR, "invalid float size");

    db->float_bytes = float_bytes;
    return DCP_SUCCESS;
}

enum dcp_rc db_write_float_size(struct dcp_db *db)
{
    unsigned size = IMM_FLOAT_BYTES;
    assert(size == 4 || size == 8);

    if (!cmp_write_u8(&db->file.ctx, (uint8_t)size))
        return error(DCP_IOERROR, "failed to write float size");

    return DCP_SUCCESS;
}

enum dcp_rc db_read_nprofiles(struct dcp_db *db)
{
    if (!cmp_read_u32(&db->file.ctx, &db->profiles.size))
        return error(DCP_IOERROR, "failed to read number of profiles");

    if (db->profiles.size > MAX_NPROFILES)
        return error(DCP_RUNTIMEERROR, "too many profiles");

    return DCP_SUCCESS;
}

static enum dcp_rc read_metadata_size(struct dcp_db *db)
{
    if (!cmp_read_u32(&db->file.ctx, &db->mt.size))
        return error(DCP_IOERROR, "failed to read metadata size");

    if (db->mt.size > max_mt_data_size())
        return error(DCP_RUNTIMEERROR, "mt.data size is too big");

    return DCP_SUCCESS;
}

/* TODO: use it somewhere to make sure they are compatible */
static enum dcp_rc check_metadata_profile_compatibility(struct dcp_db const *db)
{
    if ((db->mt.size > 0 && db->profiles.size == 0) ||
        (db->mt.size == 0 && db->profiles.size > 0))
        return error(DCP_RUNTIMEERROR, "incompatible profiles and metadata");

    return DCP_SUCCESS;
}

static void cleanup_metadata_data(struct dcp_db *db)
{
    free(db->mt.data);
    db->mt.data = NULL;
}

static enum dcp_rc read_metadata_data(struct dcp_db *db)
{
    assert(db->mt.size > 0);

    if (!(db->mt.data = malloc(sizeof(char) * db->mt.size)))
        return error(DCP_OUTOFMEM, "failed to alloc for mt.data");

    cmp_ctx_t *ctx = &db->file.ctx;
    if (!ctx->read(ctx, db->mt.data, db->mt.size))
    {
        cleanup_metadata_data(db);
        return error(DCP_IOERROR, "failed to read metadata");
    }

    if (db->mt.data[db->mt.size - 1])
    {
        cleanup_metadata_data(db);
        return error(DCP_PARSEERROR, "invalid metadata");
    }

    return DCP_SUCCESS;
}

static void cleanup_metadata_parsing(struct dcp_db *db)
{
    free(db->mt.offset);
    free(db->mt.name_length);
    db->mt.offset = NULL;
    db->mt.name_length = NULL;
}

static enum dcp_rc alloc_metadata_parsing(struct dcp_db *db)
{
    uint32_t n = db->profiles.size;

    if (!(db->mt.offset = malloc(sizeof(*db->mt.offset) * (n + 1))))
    {
        cleanup_metadata_parsing(db);
        return error(DCP_OUTOFMEM, "failed to alloc for mt.offset");
    }

    if (!(db->mt.name_length = malloc(sizeof(*db->mt.name_length) * n)))
    {
        return error(DCP_OUTOFMEM, "failed to alloc for mt.name_length");
        cleanup_metadata_parsing(db);
    }

    return DCP_SUCCESS;
}

static enum dcp_rc parse_metadata(struct dcp_db *db)
{
    assert(db->mt.offset);
    assert(db->mt.name_length);

    db->mt.offset[0] = 0;
    for (unsigned i = 0; i < db->profiles.size; ++i)
    {
        unsigned offset = db->mt.offset[i];
        unsigned j = 0;
        if (offset + j >= db->mt.size)
            return error(DCP_RUNTIMEERROR, "mt.data index overflow");

        /* Name */
        while (db->mt.data[offset + j++])
            ;
        if (j - 1 > MAX_NAME_SIZE)
            return error(DCP_ILLEGALARG, "name is too long");

        db->mt.name_length[i] = (uint8_t)(j - 1);
        if (offset + j >= db->mt.size)
            return error(DCP_RUNTIMEERROR, "mt.data index overflow");

        /* Accession */
        while (db->mt.data[offset + j++])
            ;
        db->mt.offset[i + 1] = offset + j;
    }

    return DCP_SUCCESS;
}

enum dcp_rc db_read_metadata(struct dcp_db *db)
{
    enum dcp_rc rc = DCP_SUCCESS;

    if (!(rc = read_metadata_size(db))) goto cleanup;

    if (db->mt.size > 0)
    {
        if (!(rc = read_metadata_data(db))) goto cleanup;
        if (!(rc = alloc_metadata_parsing(db))) goto cleanup;
        if ((rc = parse_metadata(db))) goto cleanup;
    }

    return DCP_SUCCESS;

cleanup:
    cleanup_metadata_data(db);
    cleanup_metadata_parsing(db);
    return rc;
}

static enum dcp_rc write_name(struct dcp_db *db, struct dcp_prof const *prof)
{
    cmp_ctx_t *ctx = &db->mt.file.ctx;

    if (!cmp_write_str(ctx, prof->mt.name, (uint32_t)strlen(prof->mt.name)))
        return error(DCP_IOERROR, "failed to write profile name");
    /* +1 for null-terminated */
    db->mt.size += strlen(prof->mt.name) + 1;

    return DCP_SUCCESS;
}

static enum dcp_rc write_accession(struct dcp_db *db,
                                   struct dcp_prof const *prof)
{
    cmp_ctx_t *ctx = &db->mt.file.ctx;

    if (!cmp_write_str(ctx, prof->mt.acc, (uint32_t)strlen(prof->mt.acc)))
        return error(DCP_IOERROR, "failed to write profile accession");
    /* +1 for null-terminated */
    db->mt.size += strlen(prof->mt.acc) + 1;

    return DCP_SUCCESS;
}

enum dcp_rc db_write_prof_meta(struct dcp_db *db, struct dcp_prof const *prof)
{
    if (prof->mt.name == NULL) return error(DCP_ILLEGALARG, "metadata not set");

    if (strlen(prof->mt.name) > MAX_NAME_SIZE)
        return error(DCP_ILLEGALARG, "profile name is too long");

    if (strlen(prof->mt.acc) > MAX_ACC_SIZE)
        return error(DCP_ILLEGALARG, "profile accession is too long");

    enum dcp_rc rc = DCP_SUCCESS;

    if ((rc = write_name(db, prof))) return rc;
    if ((rc = write_accession(db, prof))) return rc;

    return DCP_SUCCESS;
}

enum dcp_rc db_check_write_prof_ready(struct dcp_db const *db,
                                      struct dcp_prof const *prof)
{
    if (db->profiles.size == MAX_NPROFILES)
        return error(DCP_RUNTIMEERROR, "too many profiles");

    if (prof->mt.name == NULL) return error(DCP_ILLEGALARG, "metadata not set");

    return DCP_SUCCESS;
}

struct dcp_meta db_meta(struct dcp_db const *db, unsigned idx)
{
    unsigned o = db->mt.offset[idx];
    unsigned size = (unsigned)(db->mt.name_length[idx] + 1);
    return dcp_meta(db->mt.data + o, db->mt.data + o + size);
}

#if 0

#define write_imm_float(ctx, v)                                                \
    _Generic((v), float : cmp_write_float, double : cmp_write_double)(ctx, v)

struct dcp_db *dcp_db_openw(FILE *restrict fd, struct imm_abc const *abc,
                            struct dcp_cfg cfg)
{
    struct dcp_db *db = new_db();
    db->cfg = cfg;
    db->abc = *abc;
    db->file.fd = fd;
    xcmp_init(&db->file.ctx, db->file.fd);
    db->file.mode = OPEN_WRIT;

    if (!(db->mt.file.fd = tmpfile())) goto cleanup;
    xcmp_init(&db->mt.file.ctx, db->mt.file.fd);

    if (!(db->dp.fd = tmpfile())) goto cleanup;

    if (!cmp_write_u64(&db->file.ctx, MAGIC_NUMBER))
    {
        error(DCP_IOERROR, "failed to write magic number");
        goto cleanup;
    }

    if (!cmp_write_u8(&db->file.ctx, (uint8_t)cfg.prof_typeid))
    {
        error(DCP_IOERROR, "failed to write prof_type");
        goto cleanup;
    }

    unsigned float_bytes = IMM_FLOAT_BYTES;
    assert(float_bytes == 4 || float_bytes == 8);
    if (!cmp_write_u8(&db->file.ctx, (uint8_t)float_bytes))
    {
        error(DCP_IOERROR, "failed to write float_bytes");
        goto cleanup;
    }

    if (cfg.prof_typeid == DCP_STD_PROFILE)
        dcp_std_prof_init(&db->prof.std, abc);

    if (cfg.prof_typeid == DCP_PROTEIN_PROFILE)
    {
        dcp_pro_prof_init(&db->prof.pro, cfg.pro);
        if (!write_imm_float(&db->file.ctx, db->cfg.pro.epsilon))
        {
            error(DCP_IOERROR, "failed to write epsilon");
            goto cleanup;
        }

        if (!cmp_write_u8(&db->file.ctx, (uint8_t)db->cfg.pro.edist))
        {
            error(DCP_IOERROR, "failed to write entry_dist");
            goto cleanup;
        }
    }

    if (imm_abc_write(abc, db->file.fd))
    {
        error(DCP_IOERROR, "failed to write alphabet");
        goto cleanup;
    }

    if (cfg.prof_typeid == DCP_PROTEIN_PROFILE)
    {
        if (imm_abc_write(imm_super(cfg.pro.amino), db->file.fd))
        {
            error(DCP_IOERROR, "failed to write amino alphabet");
            goto cleanup;
        }
    }

    return db;

cleanup:
    fclose(db->dp.fd);
    fclose(db->mt.file.fd);
    free(db);
    return NULL;
}

struct dcp_cfg dcp_db_cfg(struct dcp_db const *db) { return db->cfg; }

enum dcp_rc dcp_db_write(struct dcp_db *db, struct dcp_prof const *prof)
{
    if (db->profiles.size == MAX_NPROFILES)
        return error(DCP_RUNTIMEERROR, "too many profiles");

    if (prof->mt.name == NULL) return error(DCP_ILLEGALARG, "metadata not set");

    if (prof->vtable.typeid == DCP_PROTEIN_PROFILE)
    {
        struct dcp_pro_prof const *p = dcp_prof_derived_c(prof);
        if (p->cfg.epsilon != db->cfg.pro.epsilon)
            return error(DCP_ILLEGALARG, "different epsilons");
        if (p->cfg.edist != db->cfg.pro.edist)
            return error(DCP_ILLEGALARG, "different entry distrs");
    }

    enum dcp_rc rc = DCP_SUCCESS;
    cmp_ctx_t *ctx = &db->mt.file.ctx;

    uint32_t len = (uint32_t)strlen(prof->mt.name);
    if (len > MAX_NAME_SIZE)
    {
        rc = error(DCP_ILLEGALARG, "name is too long");
        goto cleanup;
    }
    EWRIT_RC(!cmp_write_str(ctx, prof->mt.name, len));
    /* +1 for null-terminated */
    db->mt.size += len + 1;

    len = (uint32_t)strlen(prof->mt.acc);
    if (len > MAX_ACC_SIZE)
    {
        rc = error(DCP_ILLEGALARG, "accession is too long");
        goto cleanup;
    }
    EWRIT_RC(!cmp_write_str(ctx, prof->mt.acc, len));
    /* +1 for null-terminated */
    db->mt.size += len + 1;

    if ((rc = prof->vtable.write(prof, db->dp.fd))) goto cleanup;

    db->profiles.size++;

cleanup:
    return rc;
}

static enum dcp_rc db_closer(struct dcp_db *db);
static enum dcp_rc db_closew(struct dcp_db *db);

enum dcp_rc dcp_db_close(struct dcp_db *db)
{
    enum dcp_rc rc = DCP_SUCCESS;

    if (db->file.mode == DB_OPEN_READ)
        rc = db_closer(db);
    else if (db->file.mode == DB_OPEN_WRITE)
        rc = db_closew(db);

    free(db->mt.offset);
    free(db->mt.name_length);
    free(db->mt.data);
    db->mt.offset = NULL;
    db->mt.name_length = NULL;
    db->mt.data = NULL;
    dcp_del(&db->prof.std.super);
    free(db);
    return rc;
}

static enum dcp_rc db_closer(struct dcp_db *db) { return DCP_SUCCESS; }

static enum dcp_rc db_closew(struct dcp_db *db)
{
    enum dcp_rc rc = DCP_SUCCESS;

    EWRIT_RC(!cmp_write_u32(&db->file.ctx, db->profiles.size));

    rewind(db->mt.file.fd);
    if ((rc = flush_metadata(db))) goto cleanup;
    if (fclose(db->mt.file.fd))
    {
        rc = error(DCP_IOERROR, "failed to close metadata file");
        goto cleanup;
    }

    rewind(db->dp.fd);
    if ((rc = fcopy(db->file.fd, db->dp.fd))) goto cleanup;
    if (fclose(db->dp.fd))
    {
        rc = error(DCP_IOERROR, "failed to close DP file");
        goto cleanup;
    }

    return rc;

cleanup:
    fclose(db->mt.file.fd);
    fclose(db->dp.fd);
    return rc;
}

unsigned dcp_db_nprofiles(struct dcp_db const *db) { return db->profiles.size; }

struct dcp_meta dcp_db_meta(struct dcp_db const *db, unsigned idx)
{
    unsigned o = db->mt.offset[idx];
    unsigned size = (unsigned)(db->mt.name_length[idx] + 1);
    return dcp_meta(db->mt.data + o, db->mt.data + o + size);
}

enum dcp_rc dcp_db_read(struct dcp_db *db, struct dcp_prof *prof)
{
    if (dcp_db_end(db)) return error(DCP_RUNTIMEERROR, "end of profiles");
    prof->idx = db->profiles.curr_idx++;
    prof->mt = dcp_db_meta(db, prof->idx);
    return prof->vtable.read(prof, db->file.fd);
}
#endif

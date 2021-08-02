#include "dcp/db.h"
#include "dcp/generics.h"
#include "dcp/profile.h"
#include "imm/imm.h"
#include "profile.h"
#include "support.h"

#define MAGIC_NUMBER 0x765C806BF0E8652B

#define MAX_NPROFILES (1U << 20)

#define MAX_NAME_LENGTH 63
#define MAX_ACC_LENGTH 31

enum mode
{
    OPEN_READ = 0,
    OPEN_WRIT = 1,
};

struct dcp_db
{
    struct dcp_db_cfg cfg;
    struct imm_abc abc;
    union
    {
        struct dcp_std_profile std;
        struct dcp_pro_profile pro;
    } prof;
    struct
    {
        dcp_profile_idx_t size;
        dcp_profile_idx_t curr_idx;
    } profiles;
    struct
    {
        uint32_t *offset;
        uint8_t *name_length;
        uint32_t size;
        char *data;
        struct
        {
            FILE *fd;
            cmp_ctx_t ctx;
        } file;
    } mt;
    struct
    {
        FILE *fd;
    } dp;
    struct
    {
        FILE *fd;
        enum mode mode;
        cmp_ctx_t ctx;
    } file;
};

static inline uint32_t max_mt_data_size(void)
{
    return MAX_NPROFILES * (MAX_NAME_LENGTH + MAX_ACC_LENGTH + 2);
}

static inline struct dcp_db *new_db(void)
{
    struct dcp_db *db = xcalloc(1, sizeof(*db));
    db->abc = imm_abc_empty;
    db->profiles.size = 0;
    db->profiles.curr_idx = 0;
    db->mt.offset = NULL;
    db->mt.name_length = NULL;
    db->mt.data = NULL;
    db->mt.file.fd = NULL;
    db->dp.fd = NULL;
    db->file.fd = NULL;
    return db;
}

#define EREAD(expr)                                                            \
    do                                                                         \
    {                                                                          \
        if (!!(expr))                                                          \
        {                                                                      \
            error(IMM_IOERROR, "failed to read");                              \
            goto cleanup;                                                      \
        }                                                                      \
                                                                               \
    } while (0)

#define EREAD_RC(expr)                                                         \
    do                                                                         \
    {                                                                          \
        if (!!(expr))                                                          \
        {                                                                      \
            rc = error(IMM_IOERROR, "failed to read");                         \
            goto cleanup;                                                      \
        }                                                                      \
                                                                               \
    } while (0)

#define EWRIT(expr)                                                            \
    do                                                                         \
    {                                                                          \
        if (!!(expr))                                                          \
        {                                                                      \
            error(IMM_IOERROR, "failed to write");                             \
            goto cleanup;                                                      \
        }                                                                      \
                                                                               \
    } while (0)

#define EWRIT_RC(expr)                                                         \
    do                                                                         \
    {                                                                          \
        if (!!(expr))                                                          \
        {                                                                      \
            rc = error(IMM_IOERROR, "failed to write");                        \
            goto cleanup;                                                      \
        }                                                                      \
                                                                               \
    } while (0)

static int parse_metadata(struct dcp_db *db)
{
    int rc = IMM_SUCCESS;
    uint32_t n = db->profiles.size;

    if (!(db->mt.offset = malloc(sizeof(*db->mt.offset) * (n + 1))))
    {
        rc = error(IMM_OUTOFMEM, "failed to alloc for mt.offset");
        goto cleanup;
    }
    if (!(db->mt.name_length = malloc(sizeof(*db->mt.name_length) * n)))
    {
        rc = error(IMM_OUTOFMEM, "failed to alloc for mt.name_length");
        goto cleanup;
    }

    db->mt.offset[0] = 0;
    for (unsigned i = 0; i < n; ++i)
    {
        unsigned offset = db->mt.offset[i];
        unsigned j = 0;
        if (offset + j >= db->mt.size)
        {
            rc = error(IMM_RUNTIMEERROR, "mt.data index overflow");
            goto cleanup;
        }

        /* Name */
        while (db->mt.data[offset + j++])
            ;
        if (j - 1 > MAX_NAME_LENGTH)
        {
            rc = error(IMM_ILLEGALARG, "name is too long");
            goto cleanup;
        }
        db->mt.name_length[i] = (uint8_t)(j - 1);
        if (offset + j >= db->mt.size)
        {
            rc = error(IMM_RUNTIMEERROR, "mt.data index overflow");
            goto cleanup;
        }

        /* Accession */
        while (db->mt.data[offset + j++])
            ;
        db->mt.offset[i + 1] = offset + j;
    }

    return rc;

cleanup:
    xdel(db->mt.offset);
    xdel(db->mt.name_length);
    return rc;
}

static int read_metadata(struct dcp_db *db)
{
    int rc = IMM_SUCCESS;
    db->mt.data = NULL;

    cmp_ctx_t *ctx = &db->file.ctx;

    EREAD_RC(!cmp_read_u32(ctx, &db->mt.size));
    if ((db->mt.size > 0 && db->profiles.size == 0) ||
        (db->mt.size == 0 && db->profiles.size > 0))
    {
        rc = error(IMM_RUNTIMEERROR, "incompatible profiles and metadata");
        goto cleanup;
    }
    if (db->mt.size > max_mt_data_size())
    {
        rc = error(IMM_RUNTIMEERROR, "mt.data size is too big");
        goto cleanup;
    }

    if (db->mt.size > 0)
    {
        if (!(db->mt.data = malloc(sizeof(char) * db->mt.size)))
        {
            rc = error(IMM_OUTOFMEM, "failed to alloc for mt.data");
            goto cleanup;
        }
        EREAD_RC(!ctx->read(ctx, db->mt.data, db->mt.size));
        if (db->mt.data[db->mt.size - 1])
        {
            rc = error(IMM_PARSEERROR, "invalid mt.data");
            goto cleanup;
        }
        if ((rc = parse_metadata(db)))
            goto cleanup;
    }

    return rc;

cleanup:
    xdel(db->mt.data);
    xdel(db->mt.offset);
    xdel(db->mt.name_length);
    return rc;
}

static int flush_metadata(struct dcp_db *db)
{
    int rc = IMM_SUCCESS;
    cmp_ctx_t *ctx = &db->file.ctx;

    EWRIT_RC(!cmp_write_u32(ctx, db->mt.size));

    char name[MAX_NAME_LENGTH + 1] = {0};
    char acc[MAX_ACC_LENGTH + 1] = {0};
    for (unsigned i = 0; i < db->profiles.size; ++i)
    {
        uint32_t size = ARRAY_SIZE(name);
        EREAD_RC(!cmp_read_str(&db->mt.file.ctx, name, &size));
        EWRIT_RC(!ctx->write(ctx, name, size + 1));

        size = ARRAY_SIZE(acc);
        EREAD_RC(!cmp_read_str(&db->mt.file.ctx, acc, &size));
        EWRIT_RC(!ctx->write(ctx, acc, size + 1));
    }

    return rc;

cleanup:
    return rc;
}

struct dcp_profile *dcp_db_profile(struct dcp_db *db)
{
    return &db->prof.std.super;
}

struct dcp_db *dcp_db_openr(FILE *restrict fd)
{
    struct dcp_db *db = new_db();
    db->file.fd = fd;
    xcmp_init(&db->file.ctx, db->file.fd);
    db->file.mode = OPEN_READ;

    uint64_t magic_number = 0;
    EREAD(!cmp_read_u64(&db->file.ctx, &magic_number));
    if (magic_number != MAGIC_NUMBER)
    {
        error(IMM_PARSEERROR, "wrong file magic number");
        goto cleanup;
    }

    uint8_t prof_type = 0;
    EREAD(!cmp_read_u8(&db->file.ctx, &prof_type));
    if (prof_type != DCP_STD_PROFILE && prof_type != DCP_PROTEIN_PROFILE)
    {
        error(IMM_PARSEERROR, "wrong prof_type");
        goto cleanup;
    }
    db->cfg.prof_typeid = prof_type;

    uint8_t float_bytes = 0;
    EREAD(!cmp_read_u8(&db->file.ctx, &float_bytes));
    if (float_bytes != 4 && float_bytes != 8)
    {
        error(IMM_PARSEERROR, "wrong float_bytes");
        goto cleanup;
    }
    db->cfg.float_bytes = float_bytes;

    if (db->cfg.prof_typeid == DCP_PROTEIN_PROFILE)
    {
        if (float_bytes != 4)
        {
            float e = 0;
            EREAD(!cmp_read_float(&db->file.ctx, &e));
            db->cfg.pro.epsilon = (imm_float)e;
        }
        else
        {
            double e = 0;
            EREAD(!cmp_read_double(&db->file.ctx, &e));
            db->cfg.pro.epsilon = (imm_float)e;
        }
        if (db->cfg.pro.epsilon < 0 || db->cfg.pro.epsilon > 1)
        {
            error(IMM_PARSEERROR, "wrong epsilon");
            goto cleanup;
        }
    }

    if (imm_abc_read(&db->abc, db->file.fd))
    {
        error(IMM_IOERROR, "failed to read alphabet");
        goto cleanup;
    }

    EREAD(!cmp_read_u32(&db->file.ctx, &db->profiles.size));
    if (db->profiles.size > MAX_NPROFILES)
    {
        error(IMM_RUNTIMEERROR, "too many profiles");
        goto cleanup;
    }

    if (read_metadata(db))
        goto cleanup;

    if (db->cfg.prof_typeid == DCP_STD_PROFILE)
        dcp_std_profile_init(&db->prof.std, &db->abc);

#if 0
    if (db->cfg.prof_typeid == DCP_PROTEIN_PROFILE)
        dcp_pro_profile_init(&db->prof.pro, &db->abc);
#endif

    return db;

cleanup:
    free(db);
    return NULL;
}

#define write_imm_float(ctx, v)                                                \
    _Generic((v), float : cmp_write_float, double : cmp_write_double)(ctx, v)

struct dcp_db *dcp_db_openw(FILE *restrict fd, struct imm_abc const *abc,
                            struct dcp_db_cfg cfg)
{
    struct dcp_db *db = new_db();
    db->cfg = cfg;
    db->abc = *abc;
    db->file.fd = fd;
    xcmp_init(&db->file.ctx, db->file.fd);
    db->file.mode = OPEN_WRIT;

    if (!(db->mt.file.fd = tmpfile()))
        goto cleanup;
    xcmp_init(&db->mt.file.ctx, db->mt.file.fd);

    if (!(db->dp.fd = tmpfile()))
        goto cleanup;

    if (!cmp_write_u64(&db->file.ctx, MAGIC_NUMBER))
    {
        error(IMM_IOERROR, "failed to write magic number");
        goto cleanup;
    }

    if (!cmp_write_u8(&db->file.ctx, (uint8_t)cfg.prof_typeid))
    {
        error(IMM_IOERROR, "failed to write prof_type");
        goto cleanup;
    }

    unsigned float_bytes = IMM_FLOAT_BYTES;
    IMM_BUG(!(float_bytes == 4 || float_bytes == 8));
    if (!cmp_write_u8(&db->file.ctx, (uint8_t)float_bytes))
    {
        error(IMM_IOERROR, "failed to write float_bytes");
        goto cleanup;
    }

    if (cfg.prof_typeid == DCP_STD_PROFILE)
        dcp_std_profile_init(&db->prof.std, abc);

    if (cfg.prof_typeid == DCP_PROTEIN_PROFILE)
    {
        dcp_pro_profile_init(&db->prof.pro, cfg.pro);
        if (!write_imm_float(&db->file.ctx, db->cfg.pro.epsilon))
        {
            error(IMM_IOERROR, "failed to write epsilon");
            goto cleanup;
        }

        if (!cmp_write_u8(&db->file.ctx, (uint8_t)db->cfg.pro.edist))
        {
            error(IMM_IOERROR, "failed to write entry_dist");
            goto cleanup;
        }
    }

    if (imm_abc_write(abc, db->file.fd))
    {
        error(IMM_IOERROR, "failed to write alphabet");
        goto cleanup;
    }

    if (cfg.prof_typeid == DCP_PROTEIN_PROFILE)
    {
        if (imm_abc_write(imm_super(cfg.pro.amino), db->file.fd))
        {
            error(IMM_IOERROR, "failed to write amino alphabet");
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

struct dcp_db_cfg dcp_db_cfg(struct dcp_db const *db) { return db->cfg; }

int dcp_db_write(struct dcp_db *db, struct dcp_profile const *prof)
{
    if (db->profiles.size == MAX_NPROFILES)
    {
        return error(IMM_RUNTIMEERROR, "too many profiles");
    }
    int rc = IMM_SUCCESS;

    cmp_ctx_t *ctx = &db->mt.file.ctx;

    uint32_t len = (uint32_t)strlen(prof->mt.name);
    if (len > MAX_NAME_LENGTH)
    {
        rc = error(IMM_ILLEGALARG, "name is too long");
        goto cleanup;
    }
    EWRIT_RC(!cmp_write_str(ctx, prof->mt.name, len));
    /* +1 for null-terminated */
    db->mt.size += len + 1;

    len = (uint32_t)strlen(prof->mt.acc);
    if (len > MAX_ACC_LENGTH)
    {
        rc = error(IMM_ILLEGALARG, "accession is too long");
        goto cleanup;
    }
    EWRIT_RC(!cmp_write_str(ctx, prof->mt.acc, len));
    /* +1 for null-terminated */
    db->mt.size += len + 1;

    if ((rc = prof->vtable.write(prof, db->dp.fd)))
        goto cleanup;

    db->profiles.size++;

cleanup:
    return rc;
}

static int db_closer(struct dcp_db *db);
static int db_closew(struct dcp_db *db);

int dcp_db_close(struct dcp_db *db)
{
    int rc = IMM_SUCCESS;

    if (db->file.mode == OPEN_READ)
        rc = db_closer(db);
    else if (db->file.mode == OPEN_WRIT)
        rc = db_closew(db);

    xdel(db->mt.offset);
    xdel(db->mt.name_length);
    xdel(db->mt.data);
    dcp_del(&db->prof.std.super);
    free(db);
    return rc;
}

static int db_closer(struct dcp_db *db) { return IMM_SUCCESS; }

static int db_closew(struct dcp_db *db)
{
    int rc = IMM_SUCCESS;

    EWRIT_RC(!cmp_write_u32(&db->file.ctx, db->profiles.size));

    rewind(db->mt.file.fd);
    if ((rc = flush_metadata(db)))
        goto cleanup;
    if (fclose(db->mt.file.fd))
    {
        rc = error(IMM_IOERROR, "failed to close metadata file");
        goto cleanup;
    }

    rewind(db->dp.fd);
    if ((rc = fcopy(db->file.fd, db->dp.fd)))
        goto cleanup;
    if (fclose(db->dp.fd))
    {
        rc = error(IMM_IOERROR, "failed to close DP file");
        goto cleanup;
    }

    return rc;

cleanup:
    fclose(db->mt.file.fd);
    fclose(db->dp.fd);
    return rc;
}

struct imm_abc const *dcp_db_abc(struct dcp_db const *db) { return &db->abc; }

unsigned dcp_db_nprofiles(struct dcp_db const *db) { return db->profiles.size; }

struct dcp_meta dcp_db_meta(struct dcp_db const *db, unsigned idx)
{
    unsigned o = db->mt.offset[idx];
    unsigned size = (unsigned)(db->mt.name_length[idx] + 1);
    return dcp_meta(db->mt.data + o, db->mt.data + o + size);
}

int dcp_db_read(struct dcp_db *db, struct dcp_profile *prof)
{
    if (dcp_db_end(db))
        return error(IMM_RUNTIMEERROR, "end of profiles");
    prof->idx = db->profiles.curr_idx++;
    prof->mt = dcp_db_meta(db, prof->idx);
    return prof->vtable.read(prof, db->file.fd);
}

bool dcp_db_end(struct dcp_db const *db)
{
    return db->profiles.curr_idx >= db->profiles.size;
}

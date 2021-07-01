#include "dcp/db.h"
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
    struct imm_abc abc;
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
        char const *fp;
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
    db->file.fp = NULL;
    return db;
}

#define EREAD(expr, s)                                                         \
    do                                                                         \
    {                                                                          \
        if (!!(expr))                                                          \
        {                                                                      \
            s = error(IMM_IOERROR, "failed to read");                          \
            goto cleanup;                                                      \
        }                                                                      \
                                                                               \
    } while (0)

#define EWRIT(expr, s)                                                         \
    do                                                                         \
    {                                                                          \
        if (!!(expr))                                                          \
        {                                                                      \
            s = error(IMM_IOERROR, "failed to write");                         \
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

    EREAD(!cmp_read_u32(ctx, &db->mt.size), rc);
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
        EREAD(!ctx->read(ctx, db->mt.data, db->mt.size), rc);
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

    EWRIT(!cmp_write_u32(ctx, db->mt.size), rc);

    char name[MAX_NAME_LENGTH + 1] = {0};
    char acc[MAX_ACC_LENGTH + 1] = {0};
    for (unsigned i = 0; i < db->profiles.size; ++i)
    {
        uint32_t size = ARRAY_SIZE(name);
        EREAD(!cmp_read_str(&db->mt.file.ctx, name, &size), rc);
        EWRIT(!ctx->write(ctx, name, size + 1), rc);

        size = ARRAY_SIZE(acc);
        EREAD(!cmp_read_str(&db->mt.file.ctx, acc, &size), rc);
        EWRIT(!ctx->write(ctx, acc, size + 1), rc);
    }

    return rc;

cleanup:
    return rc;
}

struct dcp_db *dcp_db_openr(FILE *restrict fd)
{
    struct dcp_db *db = new_db();
    db->file.fd = fd;
    xcmp_init(&db->file.ctx, db->file.fd);
    db->file.mode = OPEN_READ;

    int rc = IMM_SUCCESS;

    uint64_t magic_number = 0;
    EREAD(!cmp_read_u64(&db->file.ctx, &magic_number), rc);
    if (magic_number != MAGIC_NUMBER)
    {
        rc = error(IMM_PARSEERROR, "wrong file magic number");
        goto cleanup;
    }

    if (imm_abc_read(&db->abc, db->file.fd))
    {
        rc = error(IMM_IOERROR, "failed to read alphabet");
        goto cleanup;
    }

    EREAD(!cmp_read_u32(&db->file.ctx, &db->profiles.size), rc);
    if (db->profiles.size > MAX_NPROFILES)
    {
        rc = error(IMM_RUNTIMEERROR, "too many profiles");
        goto cleanup;
    }

    if (read_metadata(db))
        goto cleanup;

    return db;

cleanup:
    free(db);
    return NULL;
}

struct dcp_db *dcp_db_openw(FILE *restrict fd, struct imm_abc const *abc)
{
    struct dcp_db *db = new_db();
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

    if (imm_abc_write(abc, db->file.fd))
    {
        error(IMM_IOERROR, "failed to write alphabet");
        goto cleanup;
    }

    return db;

cleanup:
    fclose(db->dp.fd);
    fclose(db->mt.file.fd);
    free(db);
    return NULL;
}

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
    EWRIT(!cmp_write_str(ctx, prof->mt.name, len), rc);
    /* +1 for null-terminated */
    db->mt.size += len + 1;

    len = (uint32_t)strlen(prof->mt.acc);
    if (len > MAX_ACC_LENGTH)
    {
        rc = error(IMM_ILLEGALARG, "accession is too long");
        goto cleanup;
    }
    EWRIT(!cmp_write_str(ctx, prof->mt.acc, len), rc);
    /* +1 for null-terminated */
    db->mt.size += len + 1;

    EWRIT(imm_dp_write(prof->dp.null, db->dp.fd), rc);
    EWRIT(imm_dp_write(prof->dp.alt, db->dp.fd), rc);

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
    else
        IMM_BUG(true);

    del(db->mt.offset);
    del(db->mt.name_length);
    del(db->mt.data);
    free(db);
    return rc;
}

static int db_closer(struct dcp_db *db)
{
    if (fclose(db->file.fd))
        return error(IMM_IOERROR, "failed to close file");
    return IMM_SUCCESS;
}

static int db_closew(struct dcp_db *db)
{
    int rc = IMM_SUCCESS;

    EWRIT(!cmp_write_u32(&db->file.ctx, db->profiles.size), rc);

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

    if (fclose(db->file.fd))
    {
        rc = error(IMM_IOERROR, "failed to close file");
        goto cleanup;
    }

    return rc;

cleanup:
    fclose(db->mt.file.fd);
    fclose(db->dp.fd);
    fclose(db->file.fd);
    remove(db->file.fp);
    return rc;
}

struct imm_abc const *dcp_db_abc(struct dcp_db const *db) { return &db->abc; }

unsigned dcp_db_nprofiles(struct dcp_db const *db) { return db->profiles.size; }

struct dcp_metadata dcp_db_metadata(struct dcp_db const *db, unsigned idx)
{
    unsigned o = db->mt.offset[idx];
    unsigned size = db->mt.name_length[idx] + 1;
    return dcp_metadata(db->mt.data + o, db->mt.data + o + size);
}

int dcp_db_read(struct dcp_db *db, struct dcp_profile *prof)
{
    if (dcp_db_end(db))
        return error(IMM_RUNTIMEERROR, "end of profiles");
    prof->idx = db->profiles.curr_idx++;
    prof->mt = dcp_db_metadata(db, prof->idx);
    return profile_read(prof, db->file.fd);
}

bool dcp_db_end(struct dcp_db const *db)
{
    return db->profiles.curr_idx >= db->profiles.size;
}

#include "dcp/db.h"
#include "dcp/profile.h"
#include "imm/imm.h"
#include "io.h"
#include "support.h"
#include <imm/dp.h>
#include <imm/log.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MAGIC_NUMBER 0x765C806BF0E8652B

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
    uint32_t nprofiles;
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
        struct
        {
            FILE *fd;
        } file;
    } dp;
    struct
    {
        FILE *fd;
        enum mode mode;
        cmp_ctx_t ctx;
    } file;
};

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

static void parse_metadata(struct dcp_db *db)
{
    db->mt.offset = xmalloc(sizeof(*db->mt.offset) * (db->nprofiles + 1));
    db->mt.name_length = xmalloc(sizeof(*db->mt.name_length) * db->nprofiles);

    db->mt.offset[0] = 0;
    for (unsigned i = 0; i < db->nprofiles; ++i)
    {
        unsigned offset = db->mt.offset[i];
        unsigned j = 0;

        /* Name */
        while (db->mt.data[offset + j++])
            ;
        db->mt.name_length[i] = (uint8_t)(j - 1);

        /* Accession */
        while (db->mt.data[offset + j++])
            ;
        db->mt.offset[i + 1] = offset + j;
    }
}

static int read_metadata(struct dcp_db *db)
{
    int err = IMM_SUCCESS;
    db->mt.data = NULL;

    cmp_ctx_t *ctx = &db->file.ctx;

    EREAD(!cmp_read_u32(ctx, &db->mt.size), err);
    if (db->mt.size > 0)
    {
        db->mt.data = xmalloc(sizeof(char) * db->mt.size);
        EREAD(!ctx->read(ctx, db->mt.data, db->mt.size), err);
        parse_metadata(db);
    }

    return err;

cleanup:
    xdel(db->mt.data);
    return err;
}

static int flush_metadata(struct dcp_db *db)
{
    int err = IMM_SUCCESS;
    cmp_ctx_t *ctx = &db->file.ctx;

    EWRIT(!cmp_write_u32(ctx, db->mt.size), err);

    char name[MAX_NAME_LENGTH + 1] = {0};
    char acc[MAX_ACC_LENGTH + 1] = {0};
    for (unsigned i = 0; i < db->nprofiles; ++i)
    {
        uint32_t size = ARRAY_SIZE(name);
        EREAD(!cmp_read_str(&db->mt.file.ctx, name, &size), err);
        EWRIT(!ctx->write(ctx, name, size + 1), err);

        size = ARRAY_SIZE(acc);
        EREAD(!cmp_read_str(&db->mt.file.ctx, acc, &size), err);
        EWRIT(!ctx->write(ctx, acc, size + 1), err);
    }

    return err;

cleanup:
    return err;
}

struct dcp_db *dcp_db_openr(char const *filepath)
{
    struct dcp_db *db = xcalloc(1, sizeof(*db));

    if (!(db->file.fd = fopen(filepath, "rb")))
    {
        error(IMM_IOERROR, "could not open file %s for reading", filepath);
        goto cleanup;
    }
    io_init(&db->file.ctx, db->file.fd);
    db->file.mode = OPEN_READ;

    int err = IMM_SUCCESS;

    uint64_t magic_number = 0;
    EREAD(!cmp_read_u64(&db->file.ctx, &magic_number), err);
    if (magic_number != MAGIC_NUMBER)
    {
        err = error(IMM_PARSEERROR, "wrong file magic number");
        goto cleanup;
    }

    if (imm_abc_read(&db->abc, db->file.fd))
    {
        err = error(IMM_IOERROR, "failed to read alphabet");
        goto cleanup;
    }

    EREAD(!cmp_read_u32(&db->file.ctx, &db->nprofiles), err);

    if (read_metadata(db))
        goto cleanup;

    return db;

cleanup:
    fclose(db->file.fd);
    free(db);
    return NULL;
}

struct dcp_db *dcp_db_openw(char const *filepath, struct imm_abc const *abc)
{
    struct dcp_db *db = xcalloc(1, sizeof(*db));

    if (!(db->file.fd = fopen(filepath, "wb")))
    {
        error(IMM_IOERROR, "could not open file %s for writting", filepath);
        goto cleanup;
    }
    io_init(&db->file.ctx, db->file.fd);
    db->file.mode = OPEN_WRIT;

    if (!(db->mt.file.fd = tmpfile()))
        goto cleanup;
    io_init(&db->mt.file.ctx, db->mt.file.fd);

    if (!(db->dp.file.fd = tmpfile()))
        goto cleanup;

    int err = IMM_SUCCESS;
    EWRIT(!cmp_write_u64(&db->file.ctx, MAGIC_NUMBER), err);

    if (imm_abc_write(abc, db->file.fd))
    {
        error(IMM_IOERROR, "failed to write alphabet");
        goto cleanup;
    }

    return db;

cleanup:
    fclose(db->dp.file.fd);
    fclose(db->mt.file.fd);
    fclose(db->file.fd);
    free(db);
    return NULL;
}

int dcp_db_write(struct dcp_db *db, struct dcp_profile const *prof)
{
    int err = IMM_SUCCESS;

    cmp_ctx_t *ctx = &db->mt.file.ctx;

    uint32_t len = (uint32_t)strlen(prof->mt.name);
    if (len > MAX_NAME_LENGTH)
    {
        err = error(IMM_ILLEGALARG, "too long name");
        goto cleanup;
    }
    EWRIT(!cmp_write_str(ctx, prof->mt.name, len), err);
    /* +1 for null-terminated */
    db->mt.size += len + 1;

    len = (uint32_t)strlen(prof->mt.acc);
    if (len > MAX_ACC_LENGTH)
    {
        err = error(IMM_ILLEGALARG, "too long accession");
        goto cleanup;
    }
    EWRIT(!cmp_write_str(ctx, prof->mt.acc, len), err);
    /* +1 for null-terminated */
    db->mt.size += len + 1;

    EWRIT(imm_dp_write(prof->dp.null, db->dp.file.fd), err);
    EWRIT(imm_dp_write(prof->dp.alt, db->dp.file.fd), err);

    db->nprofiles++;

cleanup:
    return err;
}

static int db_closer(struct dcp_db *db);
static int db_closew(struct dcp_db *db);

int dcp_db_close(struct dcp_db *db)
{
    int err = IMM_SUCCESS;

    if (db->file.mode == OPEN_READ)
        err = db_closer(db);
    else if (db->file.mode == OPEN_WRIT)
        err = db_closew(db);
    else
        IMM_BUG(true);

    xdel(db->mt.offset);
    xdel(db->mt.name_length);
    xdel(db->mt.data);
    free(db);
    return err;
}

static int db_closer(struct dcp_db *db)
{
    if (fclose(db->file.fd))
        return error(IMM_IOERROR, "could not close file");
    return IMM_SUCCESS;
}

static int db_closew(struct dcp_db *db)
{
    int err = IMM_SUCCESS;

    EWRIT(!cmp_write_u32(&db->file.ctx, db->nprofiles), err);

    rewind(db->mt.file.fd);
    if ((err = flush_metadata(db)))
        goto cleanup;
    if (fclose(db->mt.file.fd))
    {
        err = error(IMM_IOERROR, "could not close metadata file");
        goto cleanup;
    }

    rewind(db->dp.file.fd);
    if ((err = fcopy(db->file.fd, db->dp.file.fd)))
        goto cleanup;
    if (fclose(db->dp.file.fd))
    {
        err = error(IMM_IOERROR, "could not close DP file");
        goto cleanup;
    }

    if (fclose(db->file.fd))
        err = error(IMM_IOERROR, "could not close file");

cleanup:
    fclose(db->mt.file.fd);
    fclose(db->dp.file.fd);
    fclose(db->file.fd);
    return err;
}

struct imm_abc const *dcp_db_abc(struct dcp_db const *db) { return &db->abc; }

unsigned dcp_db_nprofiles(struct dcp_db const *db) { return db->nprofiles; }

struct dcp_metadata dcp_db_metadata(struct dcp_db const *db, unsigned idx)
{
    unsigned o = db->mt.offset[idx];
    unsigned size = db->mt.name_length[idx] + 1;
    return dcp_metadata(db->mt.data + o, db->mt.data + o + size);
}

struct dcp_profile const *dcp_db_read(struct dcp_db *db)
{
#if 0
    if (dcp_db_end(db))
        return NULL;
    struct dcp_metadata const *mt =
        profile_metadata_clone(input->profile_metadatas[input->profid]);
    return profile_create(input->profid++, nmm_input_read(input->nmm_input),
                          mt);
#endif
    return NULL;
}

bool dcp_db_end(struct dcp_db const *db)
{
    /* return db->profid >= db->nprofiles; */
    return false;
}

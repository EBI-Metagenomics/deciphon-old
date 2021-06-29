#include "dcp/db.h"
#include "dcp/profile.h"
#include "file.h"
#include "imm/imm.h"
#include "io.h"
#include "rand.h"
#include "support.h"
#include <imm/dp.h>
#include <imm/log.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MAGIC_NUMBER 0x765C806BF0E8652B

#define MAX_NAME_LENGTH 63
#define MAX_ACC_LENGTH 31

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
        struct file tmpfile;
    } mt;
    struct
    {
        struct file tmpfile;
    } dp;
    struct file file;
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
        EREAD(!cmp_read_str(&db->mt.tmpfile.ctx, name, &size), err);
        EWRIT(!ctx->write(ctx, name, size + 1), err);

        size = ARRAY_SIZE(acc);
        EREAD(!cmp_read_str(&db->mt.tmpfile.ctx, acc, &size), err);
        EWRIT(!ctx->write(ctx, acc, size + 1), err);
    }

    return err;

cleanup:
    return err;
}

struct dcp_db *dcp_db_openr(char const *filepath)
{
    struct dcp_db *db = xcalloc(1, sizeof(*db));

    if (file_open(&db->file, filepath, "rb"))
        goto cleanup;

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
    file_close(&db->file);
    free(db);
    return NULL;
}

struct dcp_db *dcp_db_openw(char const *filepath, struct imm_abc const *abc)
{
    struct dcp_db *db = xcalloc(1, sizeof(*db));
    char const *tmpfiles[2] = {tempfile(filepath), tempfile(filepath)};

    if (file_open(&db->file, filepath, "wb"))
        goto cleanup;

    if (file_open(&db->mt.tmpfile, tmpfiles[0], "w+b"))
        goto cleanup;

    if (file_open(&db->dp.tmpfile, tmpfiles[1], "w+b"))
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
    file_close(&db->dp.tmpfile);
    file_close(&db->mt.tmpfile);
    file_close(&db->file);
    free((void *)tmpfiles[0]);
    free((void *)tmpfiles[1]);
    free(db);
    return NULL;
}

int dcp_db_write(struct dcp_db *db, struct dcp_profile const *prof)
{
    int err = IMM_SUCCESS;

    cmp_ctx_t *ctx = &db->mt.tmpfile.ctx;

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

    EWRIT(imm_dp_write(prof->dp.null, db->dp.tmpfile.fd), err);
    EWRIT(imm_dp_write(prof->dp.alt, db->dp.tmpfile.fd), err);

    db->nprofiles++;

cleanup:
    return err;
}

static int db_closer(struct dcp_db *db);
static int db_closew(struct dcp_db *db);

int dcp_db_close(struct dcp_db *db)
{
    int err = IMM_SUCCESS;

    IMM_BUG(!(db->file.mode & (FILE_READ | FILE_WRIT)));

    if (db->file.mode & FILE_READ)
        err = db_closer(db);
    else
        err = db_closew(db);

    xdel(db->mt.offset);
    xdel(db->mt.name_length);
    xdel(db->mt.data);
    free(db);
    return err;
}

static int db_closer(struct dcp_db *db) { return file_close(&db->file); }

static int db_closew(struct dcp_db *db)
{
    int err = IMM_SUCCESS;

    EWRIT(!cmp_write_u32(&db->file.ctx, db->nprofiles), err);

    if ((err = file_rewind(&db->mt.tmpfile)))
        goto cleanup;
    if ((err = flush_metadata(db)))
        goto cleanup;
    if ((err = file_close(&db->mt.tmpfile)))
        goto cleanup;

    if ((err = file_rewind(&db->dp.tmpfile)))
        goto cleanup;
    if ((err = fcopy(db->file.fd, db->dp.tmpfile.fd)))
        goto cleanup;
    if ((err = file_close(&db->dp.tmpfile)))
        goto cleanup;

    err = file_close(&db->file);

cleanup:
    file_close(&db->mt.tmpfile);
    file_close(&db->dp.tmpfile);
    file_close(&db->file);
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

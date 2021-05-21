#include "dcp/db.h"
#include "dcp/profile.h"
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

#define OPEN_READ 1
#define OPEN_WRIT 2

struct file_ctx
{
    char const *filepath;
    FILE *file;
    uint8_t mode;
    cmp_ctx_t ctx;
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
        struct file_ctx tmpfile;
    } mt;
    struct
    {
        struct file_ctx tmpfile;
    } dp;
    struct file_ctx file;
};

static int open_file(struct file_ctx *f, const char *restrict filepath,
                     const char *restrict mode)
{
    f->filepath = filepath;

    if (!(f->file = fopen(filepath, mode)))
        return error(IMM_IOERROR, "could not open file %s (%s)", filepath,
                     mode);

    cmp_init(&f->ctx, f->file, file_reader, file_skipper, file_writer);

    f->mode = 0;
    if (mode[0] == 'w')
        f->mode |= OPEN_WRIT;
    if (mode[0] == 'r')
        f->mode |= OPEN_READ;
    if (strlen(mode) > 1 && mode[1] == '+')
        f->mode |= OPEN_READ;

    return IMM_SUCCESS;
}

static int close_file(struct file_ctx *f)
{
    int status = IMM_SUCCESS;
    if (f->file && fclose(f->file))
        status = error(IMM_IOERROR, "could not close file %s", f->filepath);

    f->file = NULL;
    xdel(f->filepath);
    return status;
}

static int rewind_file(struct file_ctx *f)
{
    if (fseek(f->file, 0L, SEEK_SET))
    {
        return error(IMM_IOERROR, "failed to fseek %s", f->filepath);
    }
    return IMM_SUCCESS;
}

#define EREAD(expr, s, o)                                                      \
    do                                                                         \
    {                                                                          \
        if (!!(expr))                                                          \
        {                                                                      \
            s = error(IMM_IOERROR, "failed to read");                          \
            goto o;                                                            \
        }                                                                      \
                                                                               \
    } while (0)

#define EWRIT(expr, s, o)                                                      \
    do                                                                         \
    {                                                                          \
        if (!!(expr))                                                          \
        {                                                                      \
            s = error(IMM_IOERROR, "failed to write");                         \
            goto o;                                                            \
        }                                                                      \
                                                                               \
    } while (0)

static int read_metadata(struct dcp_db *db)
{
    int status = IMM_SUCCESS;
    db->mt.data = NULL;

    cmp_ctx_t *ctx = &db->file.ctx;

    EREAD(!cmp_read_u32(ctx, &db->mt.size), status, cleanup);
    if (db->mt.size > 0)
    {
        db->mt.data = xmalloc(sizeof(char) * db->mt.size);
        EREAD(!ctx->read(ctx, db->mt.data, db->mt.size), status, cleanup);
    }

    return status;

cleanup:
    xdel(db->mt.data);
    return status;
}

static int flush_metadata(struct dcp_db *db)
{
    int status = IMM_SUCCESS;
    /* db->mt.offset = xmalloc(sizeof(*db->mt.offset) * (db->nprofiles + 1)); */
    /* db->mt.name_length = xmalloc(sizeof(*db->mt.name_length) *
     * db->nprofiles); */

    cmp_ctx_t *ctx = &db->file.ctx;

    EWRIT(!cmp_write_u32(ctx, db->mt.size), status, cleanup);

    /* db->mt.offset[0] = 0; */
    char name[MAX_NAME_LENGTH + 1] = {0};
    char acc[MAX_ACC_LENGTH + 1] = {0};
    for (unsigned i = 0; i < db->nprofiles; ++i)
    {
        /* db->mt.offset[i + 1] = db->mt.offset[i]; */

        uint32_t size = ARRAY_SIZE(name);
        EREAD(!cmp_read_str(&db->mt.tmpfile.ctx, name, &size), status, cleanup);
        EWRIT(!ctx->write(ctx, name, size + 1), status, cleanup);
        /* db->mt.offset[i + 1] += size + 1; */
        /* db->mt.name_length[i] = (uint8_t)size; */

        size = ARRAY_SIZE(acc);
        EREAD(!cmp_read_str(&db->mt.tmpfile.ctx, acc, &size), status, cleanup);
        EWRIT(!ctx->write(ctx, acc, size + 1), status, cleanup);
        /* db->mt.offset[i + 1] += size + 1; */
    }

    /* cmp_write_array(ctx, db->nprofiles + 1); */
    /* for (unsigned i = 0; i <= db->nprofiles; ++i) */
    /* { */
    /*     cmp_write_u32(ctx, db->mt.offset[i]); */
    /* } */

    /* cmp_write_array(ctx, db->nprofiles); */
    /* for (unsigned i = 0; i < db->nprofiles; ++i) */
    /* { */
    /*     cmp_write_u32(ctx, db->mt.name_length[i]); */
    /* } */

    return status;

cleanup:
    /* xdel(db->mt.offset); */
    /* xdel(db->mt.name_length); */
    return status;
}

struct dcp_db *dcp_db_openr(char const *filepath)
{
    struct dcp_db *db = xcalloc(1, sizeof(*db));

    if (open_file(&db->file, xstrdup(filepath), "rb"))
        goto cleanup;

    int status = IMM_SUCCESS;

    uint64_t magic_number = 0;
    EREAD(!cmp_read_u64(&db->file.ctx, &magic_number), status, cleanup);
    if (magic_number != MAGIC_NUMBER)
    {
        error(IMM_PARSEERROR, "wrong file magic number");
        goto cleanup;
    }

    if (imm_abc_read(&db->abc, db->file.file))
    {
        error(IMM_IOERROR, "failed to read");
        goto cleanup;
    }

    EREAD(!cmp_read_u32(&db->file.ctx, &db->nprofiles), status, cleanup);

    read_metadata(db);

    return db;

cleanup:
    free(db);
    return NULL;
}

struct dcp_db *dcp_db_openw(char const *filepath, struct imm_abc const *abc)
{
    struct dcp_db *db = xcalloc(1, sizeof(*db));

    if (open_file(&db->file, xstrdup(filepath), "wb"))
        goto cleanup;

    if (open_file(&db->mt.tmpfile, tempfile(filepath), "w+b"))
        goto cleanup;

    if (open_file(&db->dp.tmpfile, tempfile(filepath), "w+b"))
        goto cleanup;

    int status = IMM_SUCCESS;
    EWRIT(!cmp_write_u64(&db->file.ctx, MAGIC_NUMBER), status, cleanup);

    if (imm_abc_write(abc, db->file.file))
    {
        error(IMM_IOERROR, "failed to write abc");
        goto cleanup;
    }

    return db;

cleanup:
    close_file(&db->dp.tmpfile);
    close_file(&db->mt.tmpfile);
    close_file(&db->file);
    free(db);
    return NULL;
}

int dcp_db_write(struct dcp_db *db, struct dcp_profile const *prof)
{
    int status = IMM_SUCCESS;

    cmp_ctx_t *ctx = &db->mt.tmpfile.ctx;

    uint32_t len = (uint32_t)strlen(prof->mt.name);
    if (len > MAX_NAME_LENGTH)
    {
        error(IMM_ILLEGALARG, "too long name");
        goto cleanup;
    }
    EWRIT(!cmp_write_str(ctx, prof->mt.name, len), status, cleanup);
    /* +1 for null-terminated */
    db->mt.size += len + 1;

    len = (uint32_t)strlen(prof->mt.acc);
    if (len > MAX_ACC_LENGTH)
    {
        error(IMM_ILLEGALARG, "too long accession");
        goto cleanup;
    }
    EWRIT(!cmp_write_str(ctx, prof->mt.acc, len), status, cleanup);
    /* +1 for null-terminated */
    db->mt.size += len + 1;

    EWRIT(imm_dp_write(prof->dp.null, db->dp.tmpfile.file), status, cleanup);
    EWRIT(imm_dp_write(prof->dp.alt, db->dp.tmpfile.file), status, cleanup);

    db->nprofiles++;

cleanup:
    return status;
}

static int db_closer(struct dcp_db *db);
static int db_closew(struct dcp_db *db);

int dcp_db_close(struct dcp_db *db)
{
    int status = IMM_SUCCESS;

    IMM_BUG(!(db->file.mode & (OPEN_READ | OPEN_WRIT)));

    if (db->file.mode & OPEN_READ)
        status = db_closer(db);
    else
        status = db_closew(db);

    xdel(db->mt.offset);
    xdel(db->mt.name_length);
    xdel(db->mt.data);
    free(db);
    return status;
}

static int db_closer(struct dcp_db *db) { return close_file(&db->file); }

static int db_closew(struct dcp_db *db)
{
    int status = IMM_SUCCESS;

    EWRIT(!cmp_write_u32(&db->file.ctx, db->nprofiles), status, cleanup);

    rewind_file(&db->mt.tmpfile);
    if ((status = flush_metadata(db)))
        goto cleanup;

    if ((status = close_file(&db->mt.tmpfile)))
        goto cleanup;

    rewind_file(&db->dp.tmpfile);
    if ((status = fcopy(db->file.file, db->dp.tmpfile.file)))
        goto cleanup;

    if ((status = close_file(&db->dp.tmpfile)))
        goto cleanup;

    status = close_file(&db->file);

cleanup:
    close_file(&db->mt.tmpfile);
    close_file(&db->dp.tmpfile);
    close_file(&db->file);
    return status;
}

struct imm_abc const *dcp_db_abc(struct dcp_db const *db) { return &db->abc; }

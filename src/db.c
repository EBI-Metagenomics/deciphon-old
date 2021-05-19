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

enum open_mode
{
    OPEN_READ = 0,
    OPEN_WRITE = 1,
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
        char const *filepath;
        FILE *file;
    } mt;
    struct
    {
        char const *filepath;
        FILE *file;
    } dp;
    FILE *file;
    enum open_mode mode;
};

static struct dcp_db *new_db(void)
{
    struct dcp_db *db = xmalloc(sizeof(*db));
    db->nprofiles = 0;
    db->mt.offset = NULL;
    db->mt.name_length = NULL;
    db->mt.size = 0;
    db->mt.data = NULL;
    db->mt.filepath = NULL;
    db->mt.file = NULL;
    db->dp.filepath = NULL;
    db->dp.file = NULL;
    return db;
}

static int read_metadata(struct dcp_db *db, cmp_ctx_t *ctx)
{
    int status = IMM_SUCCESS;
    db->mt.data = NULL;

    if (!cmp_read_u32(ctx, &db->mt.size))
    {
        status = error(IMM_IOERROR, "failed to read");
        goto cleanup;
    }

    db->mt.data = xmalloc(sizeof(char) * db->mt.size);
    if (!ctx->read(ctx, db->mt.data, db->mt.size))
    {
        status = error(IMM_IOERROR, "failed to read");
        goto cleanup;
    }

    return status;

cleanup:
    xdel(db->mt.data);
    return status;
}

static int flush_metadata(struct dcp_db *db)
{
    int status = IMM_SUCCESS;
    db->mt.offset = xmalloc(sizeof(*db->mt.offset) * (db->nprofiles + 1));
    db->mt.name_length = xmalloc(sizeof(*db->mt.name_length) * db->nprofiles);

    if (!(db->mt.file = fopen(db->mt.filepath, "rb")))
    {
        fopen_error(db->mt.filepath);
        goto cleanup;
    }

    cmp_ctx_t ctx_tmp = {0};
    cmp_init(&ctx_tmp, db->mt.file, file_reader, file_skipper, file_writer);

    cmp_ctx_t ctx = {0};
    cmp_init(&ctx, db->file, file_reader, file_skipper, file_writer);

    cmp_write_u32(&ctx, db->mt.size);

    db->mt.offset[0] = 0;
    char name[MAX_NAME_LENGTH + 1] = {0};
    char acc[MAX_ACC_LENGTH + 1] = {0};
    for (unsigned i = 0; i < db->nprofiles; ++i)
    {
        db->mt.offset[i + 1] = db->mt.offset[i];

        uint32_t size = ARRAY_SIZE(name);
        cmp_read_str(&ctx_tmp, name, &size);
        ctx.write(&ctx, name, size + 1);
        db->mt.offset[i + 1] += size + 1;
        db->mt.name_length[i] = (uint8_t)size;

        size = ARRAY_SIZE(acc);
        cmp_read_str(&ctx_tmp, acc, &size);
        ctx.write(&ctx, acc, size + 1);
        db->mt.offset[i + 1] += size + 1;
    }

    fclose(db->mt.file);
    return status;

cleanup:
    xdel(db->mt.offset);
    xdel(db->mt.name_length);
    return status;
}

struct dcp_db *dcp_db_openr(char const *filepath)
{
    struct dcp_db *db = new_db();
    db->mode = OPEN_READ;

    if (!(db->file = fopen(filepath, "rb")))
    {
        fopen_error(filepath);
        goto cleanup;
    }

    cmp_ctx_t ctx = {0};
    cmp_init(&ctx, db->file, file_reader, file_skipper, file_writer);

    uint64_t magic_number = 0;
    if (!cmp_read_u64(&ctx, &magic_number))
    {
        error(IMM_IOERROR, "failed to read");
        goto cleanup;
    }
    if (magic_number != MAGIC_NUMBER)
    {
        error(IMM_PARSEERROR, "wrong file magic number");
        goto cleanup;
    }

    if (imm_abc_read(&db->abc, db->file))
    {
        error(IMM_IOERROR, "failed to read");
        goto cleanup;
    }

    if (!cmp_read_u32(&ctx, &db->nprofiles))
    {
        error(IMM_IOERROR, "failed to read");
        goto cleanup;
    }

    read_metadata(db, &ctx);

    return db;

cleanup:
    free(db);
    return NULL;
}

struct dcp_db *dcp_db_openw(char const *filepath, struct imm_abc const *abc)
{
    struct dcp_db *db = new_db();
    db->mt.filepath = tempfile(filepath);
    db->dp.filepath = tempfile(filepath);
    db->mode = OPEN_WRITE;

    if (!(db->file = fopen(filepath, "wb")))
    {
        fopen_error(filepath);
        goto cleanup;
    }

    if (!(db->mt.file = fopen(db->mt.filepath, "wb")))
    {
        fopen_error(db->mt.filepath);
        goto cleanup;
    }

    if (!(db->dp.file = fopen(db->dp.filepath, "wb")))
    {
        fopen_error(db->dp.filepath);
        goto cleanup;
    }

    cmp_ctx_t ctx = {0};
    cmp_init(&ctx, db->file, file_reader, file_skipper, file_writer);
    if (!cmp_write_u64(&ctx, MAGIC_NUMBER))
    {
        error(IMM_IOERROR, "failed to write magic number");
        goto cleanup;
    }

    if (imm_abc_write(abc, db->file))
    {
        error(IMM_IOERROR, "failed to write abc");
        goto cleanup;
    }

    return db;

cleanup:
    if (db->dp.file)
    {
        fclose(db->dp.file);
        db->dp.file = NULL;
    }

    if (db->mt.file)
    {
        fclose(db->mt.file);
        db->mt.file = NULL;
    }

    if (db->file)
        fclose(db->file);

    xdel(db->dp.filepath);
    xdel(db->mt.filepath);
    free(db);
    return NULL;
}

#define ERET(expr)                                                             \
    do                                                                         \
    {                                                                          \
        if (!(expr))                                                           \
            return IMM_IOERROR;                                                \
    } while (0)

int dcp_db_write(struct dcp_db *db, struct dcp_profile const *prof)
{
    cmp_ctx_t ctx = {0};
    cmp_init(&ctx, db->mt.file, file_reader, file_skipper, file_writer);
    ERET(cmp_write_str(&ctx, prof->mt.name, (uint32_t)strlen(prof->mt.name)));
    ERET(cmp_write_str(&ctx, prof->mt.acc, (uint32_t)strlen(prof->mt.acc)));
    db->mt.size += (uint32_t)strlen(prof->mt.name);
    db->mt.size += (uint32_t)strlen(prof->mt.acc);

    ERET(!imm_dp_write(prof->dp.null, db->dp.file));
    ERET(!imm_dp_write(prof->dp.alt, db->dp.file));

    db->nprofiles++;
    return IMM_SUCCESS;
}

static int db_closer(struct dcp_db *db);
static int db_closew(struct dcp_db *db);

int dcp_db_close(struct dcp_db *db)
{
    int status = IMM_SUCCESS;

    if (db->mode == OPEN_READ)
        status = db_closer(db);

    if (db->mode == OPEN_WRITE)
        status = db_closew(db);

    free(db);
    return status;
}

static int db_closer(struct dcp_db *db)
{
    int status = IMM_SUCCESS;
    if (fclose(db->file))
        status = error(IMM_IOERROR, "could not close file");

    return status;
}

static int db_closew(struct dcp_db *db)
{
    int status = IMM_SUCCESS;

    if (db->mt.file && fclose(db->mt.file))
        status = error(IMM_IOERROR, "could not close file");

    if (db->dp.file && fclose(db->dp.file))
        status = error(IMM_IOERROR, "could not close file");

    cmp_ctx_t ctx = {0};
    cmp_init(&ctx, db->file, file_reader, file_skipper, file_writer);
    cmp_write_u32(&ctx, db->nprofiles);

    flush_metadata(db);

    if (fclose(db->file))
        status = error(IMM_IOERROR, "could not close file");

    xdel(db->dp.filepath);
    xdel(db->mt.filepath);
    return status;
}

struct imm_abc const *dcp_db_abc(struct dcp_db const *db) { return &db->abc; }

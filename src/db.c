#include "dcp/db.h"
#include "imm/imm.h"
#include "io.h"
#include "support.h"
#include <imm/log.h>

#define MAGIC_NUMBER 0x765C806BF0E8652B

struct dcp_db
{
    struct imm_abc abc;
    uint32_t nprofiles;
    struct
    {
        uint32_t *offset;
        uint8_t *name_length;
        char *data;
    } metadata;
    FILE *file;
};

struct dcp_db *dcp_db_openr(char const *filepath)
{
    struct dcp_db *db = xmalloc(sizeof(*db));

    if (!(db->file = fopen(filepath, "rb")))
    {
        error(IMM_IOERROR, "could not open file %s for reading", filepath);
        goto cleanup;
    }

    cmp_ctx_t cmp = {0};
    cmp_init(&cmp, db->file, file_reader, file_skipper, file_writer);

    uint64_t magic_number;
    cmp_read_u64(&cmp, &magic_number);
    if (magic_number != MAGIC_NUMBER)
    {
        error(IMM_PARSEERROR, "wrong file magic number");
        goto cleanup;
    }

    if (imm_abc_read(&db->abc, db->file))
    {
        error(IMM_IOERROR, "failed to read abc");
        goto cleanup;
    }

    return db;

cleanup:
    free(db);
    return NULL;
}

struct dcp_db *dcp_db_openw(char const *filepath, struct imm_abc const *abc)
{
    struct dcp_db *db = xmalloc(sizeof(*db));

    if (!(db->file = fopen(filepath, "wb")))
    {
        error(IMM_IOERROR, "could not open file %s for writing", filepath);
        goto cleanup;
    }

    cmp_ctx_t cmp = {0};
    cmp_init(&cmp, db->file, file_reader, file_skipper, file_writer);
    cmp_write_u64(&cmp, MAGIC_NUMBER);

    if (imm_abc_write(abc, db->file))
    {
        error(IMM_IOERROR, "failed to write abc");
        goto cleanup;
    }

    return db;

cleanup:
    free(db);
    return NULL;
}

int dcp_db_write(struct dcp_db *db, struct dcp_profile const *prof) {

}

int dcp_db_close(struct dcp_db *db)
{
    int status = IMM_SUCCESS;
    if (fclose(db->file))
        status = error(IMM_IOERROR, "could not close file");

    free(db);
    return status;
}

struct imm_abc const *dcp_db_abc(struct dcp_db const *db) { return &db->abc; }

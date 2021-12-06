#include "db.h"
#include "cmp/cmp.h"
#include "dcp_limits.h"
#include "imm/imm.h"
#include "logger.h"
#include "macros.h"
#include "profile.h"
#include "xfile.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define MAGIC_NUMBER 0x765C806BF0E8652B

struct db const db_default = {0};

static enum rc init_tmpmeta(struct db *db)
{
    FILE *fd = tmpfile();
    if (!fd) return error(RC_IOERROR, "tmpfile() failed");
    cmp_setup(&db->mt.file.cmp, fd);
    return RC_DONE;
}

static enum rc init_tmpdp(struct db *db)
{
    FILE *fd = tmpfile();
    if (!fd) return error(RC_IOERROR, "tmpfile() failed");
    cmp_setup(&db->dp.cmp, fd);
    return RC_DONE;
}

void db_init(struct db *db, enum profile_typeid prof_typeid)
{
    db->prof_typeid = prof_typeid;
    db->float_size = IMM_FLOAT_BYTES;
    db->npartitions = 1;
    memset(db->partition_offset, 0, MEMBER_SIZE(*db, partition_offset));
    db->profiles.size = 0;
    db->profiles.curr_idx = 0;
    db->mt.offset = NULL;
    db->mt.name_length = NULL;
    db->mt.size = 0;
    db->mt.data = NULL;
    cmp_setup(&db->mt.file.cmp, NULL);
    cmp_setup(&db->dp.cmp, NULL);
    unsigned n = ARRAY_SIZE(MEMBER_REF(*db, partition_offset));
    for (unsigned i = 0; i < n; ++i)
        cmp_setup(&db->file.cmp[i], NULL);
    db->file.mode = DB_OPEN_NULL;
}

void db_openr(struct db *db, FILE *restrict fp)
{
    cmp_setup(&db->file.cmp[0], fp);
    db->file.mode = DB_OPEN_READ;
}

void db_set_files(struct db *db, unsigned nfiles, FILE *restrict fp[])
{
    for (unsigned i = 0; i < nfiles; ++i)
        cmp_setup(&db->file.cmp[i], fp[i]);
}

enum rc db_openw(struct db *db, FILE *restrict fp)
{
    enum rc rc = init_tmpdp(db);
    if (rc) return rc;

    cmp_setup(&db->file.cmp[0], fp);
    db->file.mode = DB_OPEN_WRITE;

    return init_tmpmeta(db);
}

static enum rc flush_metadata(struct db *db)
{
    struct cmp_ctx_s *cmp = &db->file.cmp[0];

    if (!cmp_write_u32(cmp, db->mt.size))
        return error(RC_IOERROR, "failed to write metadata size");

    char name[DCP_PROFILE_NAME_SIZE] = {0};
    char acc[DCP_PROFILE_ACC_SIZE] = {0};
    for (unsigned i = 0; i < db->profiles.size; ++i)
    {
        uint32_t len = ARRAY_SIZE(name) - 1;
        if (!cmp_read_cstr(&db->mt.file.cmp, name, &len))
            return error(RC_IOERROR, "failed to read name size");

        /* write the null-terminated character */
        if (!cmp_write(cmp, name, len + 1))
            return error(RC_IOERROR, "failed to write name");

        len = ARRAY_SIZE(acc) - 1;
        if (!cmp_read_cstr(&db->mt.file.cmp, acc, &len))
            return error(RC_IOERROR, "failed to read acc size");

        /* write the null-terminated character */
        if (!cmp_write(cmp, acc, len + 1))
            return error(RC_IOERROR, "failed to write acc");
    }

    return RC_DONE;
}

static void cleanup_metadata_data(struct db *db)
{
    free(db->mt.data);
    db->mt.data = NULL;
}

static void cleanup_metadata_parsing(struct db *db)
{
    free(db->mt.offset);
    free(db->mt.name_length);
    db->mt.offset = NULL;
    db->mt.name_length = NULL;
}

static void closer(struct db *db)
{
    assert(db->file.mode == DB_OPEN_READ);
    cleanup_metadata_data(db);
    cleanup_metadata_parsing(db);
}

static enum rc closew(struct db *db)
{
    assert(db->file.mode == DB_OPEN_WRITE);

    if (!cmp_write_u32(&db->file.cmp[0], db->profiles.size))
        return error(RC_IOERROR, "failed to write number of profiles");

    rewind(cmp_file(&db->mt.file.cmp));
    enum rc rc = RC_DONE;
    if ((rc = flush_metadata(db))) goto cleanup;
    if (fclose(cmp_file(&db->mt.file.cmp)))
    {
        rc = error(RC_IOERROR, "failed to close metadata file");
        goto cleanup;
    }

    rewind(cmp_file(&db->dp.cmp));
    if ((rc = xfile_copy(cmp_file(&db->file.cmp[0]), cmp_file(&db->dp.cmp))))
        goto cleanup;

    if (fclose(cmp_file(&db->dp.cmp)))
    {
        rc = error(RC_IOERROR, "failed to close DP file");
        goto cleanup;
    }

    return rc;

cleanup:
    fclose(cmp_file(&db->mt.file.cmp));
    fclose(cmp_file(&db->dp.cmp));
    return rc;
}

enum rc db_close(struct db *db)
{
    if (db->file.mode == DB_OPEN_WRITE) return closew(db);
    closer(db);
    return RC_DONE;
}

bool db_end(struct db const *db)
{
    return db->profiles.curr_idx >= db->profiles.size;
}

static inline uint32_t max_mt_data_size(void)
{
    return DCP_MAX_NPROFILES * (DCP_PROFILE_NAME_SIZE + DCP_PROFILE_ACC_SIZE);
}

enum rc db_read_magic_number(struct db *db)
{
    uint64_t magic_number = 0;
    if (!cmp_read_u64(&db->file.cmp[0], &magic_number))
        return error(RC_IOERROR, "failed to read magic number");

    if (magic_number != MAGIC_NUMBER)
        return error(RC_PARSEERROR, "wrong file magic number");

    return RC_DONE;
}

enum rc db_write_magic_number(struct db *db)
{
    if (!cmp_write_u64(&db->file.cmp[0], MAGIC_NUMBER))
        return error(RC_IOERROR, "failed to write magic number");

    return RC_DONE;
}

enum rc db_read_prof_type(struct db *db)
{
    uint8_t prof_type = 0;
    if (!cmp_read_u8(&db->file.cmp[0], &prof_type))
        return error(RC_IOERROR, "failed to read profile type");

    if (prof_type != STANDARD_PROFILE && prof_type != PROTEIN_PROFILE)
        return error(RC_PARSEERROR, "wrong prof_type");

    db->prof_typeid = prof_type;
    return RC_DONE;
}

enum rc db_write_prof_type(struct db *db)
{
    if (!cmp_write_u8(&db->file.cmp[0], (uint8_t)db->prof_typeid))
        return error(RC_IOERROR, "failed to write prof_type");

    return RC_DONE;
}

enum rc db_read_float_size(struct db *db)
{
    uint8_t float_bytes = 0;
    if (!cmp_read_u8(&db->file.cmp[0], &float_bytes))
        return error(RC_IOERROR, "failed to read float size");

    if (float_bytes != 4 && float_bytes != 8)
        return error(RC_PARSEERROR, "invalid float size");

    db->float_size = float_bytes;
    return RC_DONE;
}

enum rc db_write_float_size(struct db *db)
{
    unsigned size = IMM_FLOAT_BYTES;
    assert(size == 4 || size == 8);

    if (!cmp_write_u8(&db->file.cmp[0], (uint8_t)size))
        return error(RC_IOERROR, "failed to write float size");

    return RC_DONE;
}

enum rc db_read_nprofiles(struct db *db)
{
    if (!cmp_read_u32(&db->file.cmp[0], &db->profiles.size))
        return error(RC_IOERROR, "failed to read number of profiles");

    if (db->profiles.size > DCP_MAX_NPROFILES)
        return error(RC_FAIL, "too many profiles");

    return RC_DONE;
}

static enum rc read_metadata_size(struct db *db)
{
    if (!cmp_read_u32(&db->file.cmp[0], &db->mt.size))
        return error(RC_IOERROR, "failed to read metadata size");

    if (db->mt.size > max_mt_data_size())
        return error(RC_FAIL, "mt.data size is too big");

    return RC_DONE;
}

/* TODO: use it somewhere to make sure they are compatible */
static enum rc check_metadata_profile_compatibility(struct db const *db)
{
    if ((db->mt.size > 0 && db->profiles.size == 0) ||
        (db->mt.size == 0 && db->profiles.size > 0))
        return error(RC_FAIL, "incompatible profiles and metadata");

    return RC_DONE;
}

static enum rc read_metadata_data(struct db *db)
{
    assert(db->mt.size > 0);

    if (!(db->mt.data = malloc(sizeof(char) * db->mt.size)))
        return error(RC_OUTOFMEM, "failed to alloc for mt.data");

    struct cmp_ctx_s *cmp = &db->file.cmp[0];
    if (!cmp_read(cmp, db->mt.data, db->mt.size))
    {
        cleanup_metadata_data(db);
        return error(RC_IOERROR, "failed to read metadata");
    }

    if (db->mt.data[db->mt.size - 1])
    {
        cleanup_metadata_data(db);
        return error(RC_PARSEERROR, "invalid metadata");
    }

    return RC_DONE;
}

static enum rc alloc_metadata_parsing(struct db *db)
{
    uint32_t n = db->profiles.size;

    if (!(db->mt.offset = malloc(sizeof(*db->mt.offset) * (n + 1))))
    {
        cleanup_metadata_parsing(db);
        return error(RC_OUTOFMEM, "failed to alloc for mt.offset");
    }

    if (!(db->mt.name_length = malloc(sizeof(*db->mt.name_length) * n)))
    {
        return error(RC_OUTOFMEM, "failed to alloc for mt.name_length");
        cleanup_metadata_parsing(db);
    }

    return RC_DONE;
}

static enum rc parse_metadata(struct db *db)
{
    assert(db->mt.offset);
    assert(db->mt.name_length);

    db->mt.offset[0] = 0;
    for (unsigned i = 0; i < db->profiles.size; ++i)
    {
        unsigned offset = db->mt.offset[i];
        unsigned j = 0;
        if (offset + j >= db->mt.size)
            return error(RC_FAIL, "mt.data index overflow");

        /* Name */
        while (db->mt.data[offset + j++])
            ;
        if (j > DCP_PROFILE_NAME_SIZE)
            return error(RC_ILLEGALARG, "name is too long");

        db->mt.name_length[i] = (uint8_t)(j - 1);
        if (offset + j >= db->mt.size)
            return error(RC_FAIL, "mt.data index overflow");

        /* Accession */
        while (db->mt.data[offset + j++])
            ;
        db->mt.offset[i + 1] = offset + j;
    }

    return RC_DONE;
}

enum rc db_read_metadata(struct db *db)
{
    enum rc rc = RC_DONE;

    if ((rc = read_metadata_size(db))) goto cleanup;

    if (db->mt.size > 0)
    {
        if ((rc = read_metadata_data(db))) goto cleanup;
        if ((rc = alloc_metadata_parsing(db))) goto cleanup;
        if ((rc = parse_metadata(db))) goto cleanup;
    }

    return RC_DONE;

cleanup:
    cleanup_metadata_data(db);
    cleanup_metadata_parsing(db);
    return rc;
}

static enum rc write_name(struct db *db, struct profile const *prof)
{
    struct cmp_ctx_s *ctx = &db->mt.file.cmp;

    if (!cmp_write_str(ctx, prof->mt.name, (uint32_t)strlen(prof->mt.name)))
        return error(RC_IOERROR, "failed to write profile name");
    /* +1 for null-terminated */
    db->mt.size += (uint32_t)strlen(prof->mt.name) + 1;

    return RC_DONE;
}

static enum rc write_accession(struct db *db, struct profile const *prof)
{
    struct cmp_ctx_s *ctx = &db->mt.file.cmp;

    if (!cmp_write_str(ctx, prof->mt.acc, (uint32_t)strlen(prof->mt.acc)))
        return error(RC_IOERROR, "failed to write profile accession");
    /* +1 for null-terminated */
    db->mt.size += (uint32_t)strlen(prof->mt.acc) + 1;

    return RC_DONE;
}

enum rc db_write_prof_meta(struct db *db, struct profile const *prof)
{
    if (prof->mt.name == NULL) return error(RC_ILLEGALARG, "metadata not set");

    if (strlen(prof->mt.name) >= DCP_PROFILE_NAME_SIZE)
        return error(RC_ILLEGALARG, "profile name is too long");

    if (strlen(prof->mt.acc) >= DCP_PROFILE_ACC_SIZE)
        return error(RC_ILLEGALARG, "profile accession is too long");

    enum rc rc = RC_DONE;

    if ((rc = write_name(db, prof))) return rc;
    if ((rc = write_accession(db, prof))) return rc;

    return RC_DONE;
}

enum rc db_check_write_prof_ready(struct db const *db,
                                  struct profile const *prof)
{
    if (db->profiles.size == DCP_MAX_NPROFILES)
        return error(RC_FAIL, "too many profiles");

    if (prof->mt.name == NULL) return error(RC_ILLEGALARG, "metadata not set");

    return RC_DONE;
}

struct meta db_meta(struct db const *db, unsigned idx)
{
    unsigned o = db->mt.offset[idx];
    unsigned size = (unsigned)(db->mt.name_length[idx] + 1);
    return meta(db->mt.data + o, db->mt.data + o + size);
}

unsigned db_float_size(struct db const *db) { return db->float_size; }

enum profile_typeid db_prof_typeid(struct db const *db)
{
    return db->prof_typeid;
}

enum rc db_current_offset(struct db *db, off_t *offset)
{
    FILE *fp = cmp_file(&db->file.cmp[0]);
    if ((*offset = ftello(fp)) == -1)
        return error(RC_IOERROR, "failed to ftello");
    return RC_DONE;
}

enum rc db_record_first_partition_offset(struct db *db)
{
    return db_current_offset(db, db->partition_offset);
}

enum rc db_rewind(struct db *db)
{
    for (unsigned i = 0; i < db->npartitions; ++i)
    {
        FILE *fp = cmp_file(&db->file.cmp[i]);
        if (fseek(fp, db->partition_offset[i], SEEK_SET) == -1)
            return error(RC_IOERROR, "failed to fseek");
    }
    db->profiles.curr_idx = 0;
    return RC_DONE;
}

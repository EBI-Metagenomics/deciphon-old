#include "db.h"
#include "dcp/db.h"
#include "dcp/generics.h"
#include "dcp/prof.h"
#include "error.h"
#include "imm/imm.h"
#include "macros.h"
#include "prof.h"
#include "third-party/cmp.h"
#include "xcmp.h"
#include "xfile.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define MAGIC_NUMBER 0x765C806BF0E8652B

#define MAX_NPROFILES (1U << 20)

#define MAX_NAME_SIZE 63
#define MAX_ACC_SIZE 31

struct dcp_db const dcp_db_default = {0};

static enum dcp_rc init_tmpmeta(struct dcp_db *db)
{
    FILE *fd = tmpfile();
    if (!fd) return error(DCP_IOERROR, "tmpfile() failed");
    xcmp_setup(&db->mt.file.cmp, fd);
    return DCP_DONE;
}

static enum dcp_rc init_tmpdp(struct dcp_db *db)
{
    FILE *fd = tmpfile();
    if (!fd) return error(DCP_IOERROR, "tmpfile() failed");
    xcmp_setup(&db->dp.cmp, fd);
    return DCP_DONE;
}

void db_init(struct dcp_db *db, enum dcp_prof_typeid prof_typeid)
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
    db->mt.file.cmp = xcmp_init(NULL);
    db->dp.cmp = xcmp_init(NULL);
    unsigned n = ARRAY_SIZE(MEMBER_REF(*db, partition_offset));
    for (unsigned i = 0; i < n; ++i)
        db->file.cmp[i] = xcmp_init(NULL);
    db->file.mode = DB_OPEN_NULL;
}

void db_openr(struct dcp_db *db, FILE *restrict fp)
{
    xcmp_setup(&db->file.cmp[0], fp);
    db->file.mode = DB_OPEN_READ;
}

void db_set_files(struct dcp_db *db, unsigned nfiles, FILE *restrict fp[])
{
    for (unsigned i = 0; i < nfiles; ++i)
        xcmp_setup(&db->file.cmp[i], fp[i]);
}

enum dcp_rc db_openw(struct dcp_db *db, FILE *restrict fp)
{
    enum dcp_rc rc = init_tmpdp(db);
    if (rc) return rc;

    xcmp_setup(&db->file.cmp[0], fp);
    db->file.mode = DB_OPEN_WRITE;

    return init_tmpmeta(db);
}

static enum dcp_rc flush_metadata(struct dcp_db *db)
{
    struct cmp_ctx_s *cmp = &db->file.cmp[0];

    if (!cmp_write_u32(cmp, db->mt.size))
        return error(DCP_IOERROR, "failed to write metadata size");

    char name[MAX_NAME_SIZE + 1] = {0};
    char acc[MAX_ACC_SIZE + 1] = {0};
    for (unsigned i = 0; i < db->profiles.size; ++i)
    {
        uint32_t size = ARRAY_SIZE(name);
        if (!cmp_read_str(&db->mt.file.cmp, name, &size))
            return error(DCP_IOERROR, "failed to read name size");

        if (!xcmp_write(cmp, name, size + 1))
            return error(DCP_IOERROR, "failed to write name");

        size = ARRAY_SIZE(acc);
        if (!cmp_read_str(&db->mt.file.cmp, acc, &size))
            return error(DCP_IOERROR, "failed to read acc size");

        if (!xcmp_write(cmp, acc, size + 1))
            return error(DCP_IOERROR, "failed to write acc");
    }

    return DCP_DONE;
}

static void cleanup_metadata_data(struct dcp_db *db)
{
    free(db->mt.data);
    db->mt.data = NULL;
}

static void cleanup_metadata_parsing(struct dcp_db *db)
{
    free(db->mt.offset);
    free(db->mt.name_length);
    db->mt.offset = NULL;
    db->mt.name_length = NULL;
}

static void closer(struct dcp_db *db)
{
    assert(db->file.mode == DB_OPEN_READ);
    cleanup_metadata_data(db);
    cleanup_metadata_parsing(db);
}

static enum dcp_rc closew(struct dcp_db *db)
{
    assert(db->file.mode == DB_OPEN_WRITE);

    if (!cmp_write_u32(&db->file.cmp[0], db->profiles.size))
        return error(DCP_IOERROR, "failed to write number of profiles");

    xcmp_rewind(&db->mt.file.cmp);
    enum dcp_rc rc = DCP_DONE;
    if ((rc = flush_metadata(db))) goto cleanup;
    if (xcmp_close(&db->mt.file.cmp))
    {
        rc = error(DCP_IOERROR, "failed to close metadata file");
        goto cleanup;
    }

    xcmp_rewind(&db->dp.cmp);
    if ((rc = xfile_copy(xcmp_fp(&db->file.cmp[0]), xcmp_fp(&db->dp.cmp))))
        goto cleanup;

    if (xcmp_close(&db->dp.cmp))
    {
        rc = error(DCP_IOERROR, "failed to close DP file");
        goto cleanup;
    }

    return rc;

cleanup:
    xcmp_close(&db->mt.file.cmp);
    xcmp_close(&db->dp.cmp);
    return rc;
}

enum dcp_rc db_close(struct dcp_db *db)
{
    if (db->file.mode == DB_OPEN_WRITE) return closew(db);
    closer(db);
    return DCP_DONE;
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
    if (!cmp_read_u64(&db->file.cmp[0], &magic_number))
        return error(DCP_IOERROR, "failed to read magic number");

    if (magic_number != MAGIC_NUMBER)
        return error(DCP_PARSEERROR, "wrong file magic number");

    return DCP_DONE;
}

enum dcp_rc db_write_magic_number(struct dcp_db *db)
{
    if (!cmp_write_u64(&db->file.cmp[0], MAGIC_NUMBER))
        return error(DCP_IOERROR, "failed to write magic number");

    return DCP_DONE;
}

enum dcp_rc db_read_prof_type(struct dcp_db *db)
{
    uint8_t prof_type = 0;
    if (!cmp_read_u8(&db->file.cmp[0], &prof_type))
        return error(DCP_IOERROR, "failed to read profile type");

    if (prof_type != DCP_STD_PROFILE && prof_type != DCP_PRO_PROFILE)
        return error(DCP_PARSEERROR, "wrong prof_type");

    db->prof_typeid = prof_type;
    return DCP_DONE;
}

enum dcp_rc db_write_prof_type(struct dcp_db *db)
{
    if (!cmp_write_u8(&db->file.cmp[0], (uint8_t)db->prof_typeid))
        return error(DCP_IOERROR, "failed to write prof_type");

    return DCP_DONE;
}

enum dcp_rc db_read_float_size(struct dcp_db *db)
{
    uint8_t float_bytes = 0;
    if (!cmp_read_u8(&db->file.cmp[0], &float_bytes))
        return error(DCP_IOERROR, "failed to read float size");

    if (float_bytes != 4 && float_bytes != 8)
        return error(DCP_PARSEERROR, "invalid float size");

    db->float_size = float_bytes;
    return DCP_DONE;
}

enum dcp_rc db_write_float_size(struct dcp_db *db)
{
    unsigned size = IMM_FLOAT_BYTES;
    assert(size == 4 || size == 8);

    if (!cmp_write_u8(&db->file.cmp[0], (uint8_t)size))
        return error(DCP_IOERROR, "failed to write float size");

    return DCP_DONE;
}

enum dcp_rc db_read_nprofiles(struct dcp_db *db)
{
    if (!cmp_read_u32(&db->file.cmp[0], &db->profiles.size))
        return error(DCP_IOERROR, "failed to read number of profiles");

    if (db->profiles.size > MAX_NPROFILES)
        return error(DCP_FAIL, "too many profiles");

    return DCP_DONE;
}

static enum dcp_rc read_metadata_size(struct dcp_db *db)
{
    if (!cmp_read_u32(&db->file.cmp[0], &db->mt.size))
        return error(DCP_IOERROR, "failed to read metadata size");

    if (db->mt.size > max_mt_data_size())
        return error(DCP_FAIL, "mt.data size is too big");

    return DCP_DONE;
}

/* TODO: use it somewhere to make sure they are compatible */
static enum dcp_rc check_metadata_profile_compatibility(struct dcp_db const *db)
{
    if ((db->mt.size > 0 && db->profiles.size == 0) ||
        (db->mt.size == 0 && db->profiles.size > 0))
        return error(DCP_FAIL, "incompatible profiles and metadata");

    return DCP_DONE;
}

static enum dcp_rc read_metadata_data(struct dcp_db *db)
{
    assert(db->mt.size > 0);

    if (!(db->mt.data = malloc(sizeof(char) * db->mt.size)))
        return error(DCP_OUTOFMEM, "failed to alloc for mt.data");

    struct cmp_ctx_s *cmp = &db->file.cmp[0];
    if (!xcmp_read(cmp, db->mt.data, db->mt.size))
    {
        cleanup_metadata_data(db);
        return error(DCP_IOERROR, "failed to read metadata");
    }

    if (db->mt.data[db->mt.size - 1])
    {
        cleanup_metadata_data(db);
        return error(DCP_PARSEERROR, "invalid metadata");
    }

    return DCP_DONE;
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

    return DCP_DONE;
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
            return error(DCP_FAIL, "mt.data index overflow");

        /* Name */
        while (db->mt.data[offset + j++])
            ;
        if (j - 1 > MAX_NAME_SIZE)
            return error(DCP_ILLEGALARG, "name is too long");

        db->mt.name_length[i] = (uint8_t)(j - 1);
        if (offset + j >= db->mt.size)
            return error(DCP_FAIL, "mt.data index overflow");

        /* Accession */
        while (db->mt.data[offset + j++])
            ;
        db->mt.offset[i + 1] = offset + j;
    }

    return DCP_DONE;
}

enum dcp_rc db_read_metadata(struct dcp_db *db)
{
    enum dcp_rc rc = DCP_DONE;

    if ((rc = read_metadata_size(db))) goto cleanup;

    if (db->mt.size > 0)
    {
        if ((rc = read_metadata_data(db))) goto cleanup;
        if ((rc = alloc_metadata_parsing(db))) goto cleanup;
        if ((rc = parse_metadata(db))) goto cleanup;
    }

    return DCP_DONE;

cleanup:
    cleanup_metadata_data(db);
    cleanup_metadata_parsing(db);
    return rc;
}

static enum dcp_rc write_name(struct dcp_db *db, struct dcp_prof const *prof)
{
    struct cmp_ctx_s *ctx = &db->mt.file.cmp;

    if (!cmp_write_str(ctx, prof->mt.name, (uint32_t)strlen(prof->mt.name)))
        return error(DCP_IOERROR, "failed to write profile name");
    /* +1 for null-terminated */
    db->mt.size += (uint32_t)strlen(prof->mt.name) + 1;

    return DCP_DONE;
}

static enum dcp_rc write_accession(struct dcp_db *db,
                                   struct dcp_prof const *prof)
{
    struct cmp_ctx_s *ctx = &db->mt.file.cmp;

    if (!cmp_write_str(ctx, prof->mt.acc, (uint32_t)strlen(prof->mt.acc)))
        return error(DCP_IOERROR, "failed to write profile accession");
    /* +1 for null-terminated */
    db->mt.size += (uint32_t)strlen(prof->mt.acc) + 1;

    return DCP_DONE;
}

enum dcp_rc db_write_prof_meta(struct dcp_db *db, struct dcp_prof const *prof)
{
    if (prof->mt.name == NULL) return error(DCP_ILLEGALARG, "metadata not set");

    if (strlen(prof->mt.name) > MAX_NAME_SIZE)
        return error(DCP_ILLEGALARG, "profile name is too long");

    if (strlen(prof->mt.acc) > MAX_ACC_SIZE)
        return error(DCP_ILLEGALARG, "profile accession is too long");

    enum dcp_rc rc = DCP_DONE;

    if ((rc = write_name(db, prof))) return rc;
    if ((rc = write_accession(db, prof))) return rc;

    return DCP_DONE;
}

enum dcp_rc db_check_write_prof_ready(struct dcp_db const *db,
                                      struct dcp_prof const *prof)
{
    if (db->profiles.size == MAX_NPROFILES)
        return error(DCP_FAIL, "too many profiles");

    if (prof->mt.name == NULL) return error(DCP_ILLEGALARG, "metadata not set");

    return DCP_DONE;
}

struct dcp_meta db_meta(struct dcp_db const *db, unsigned idx)
{
    unsigned o = db->mt.offset[idx];
    unsigned size = (unsigned)(db->mt.name_length[idx] + 1);
    return dcp_meta(db->mt.data + o, db->mt.data + o + size);
}

unsigned dcp_db_float_size(struct dcp_db const *db) { return db->float_size; }

enum dcp_prof_typeid dcp_db_prof_typeid(struct dcp_db const *db)
{
    return db->prof_typeid;
}

unsigned dcp_db_nprofiles(struct dcp_db const *db) { return db->profiles.size; }

struct dcp_meta dcp_db_meta(struct dcp_db const *db, unsigned idx)
{
    return db_meta(db, idx);
}

bool dcp_db_end(struct dcp_db const *db) { return db_end(db); }

enum dcp_rc db_current_offset(struct dcp_db *db, off_t *offset)
{
    FILE *fp = xcmp_fp(&db->file.cmp[0]);
    if ((*offset = ftello(fp)) == -1)
        return error(DCP_IOERROR, "failed to ftello");
    return DCP_DONE;
}

enum dcp_rc db_record_first_partition_offset(struct dcp_db *db)
{
    return db_current_offset(db, db->partition_offset);
}

enum dcp_rc db_rewind(struct dcp_db *db)
{
    for (unsigned i = 0; i < db->npartitions; ++i)
    {
        FILE *fp = xcmp_fp(&db->file.cmp[i]);
        if (fseek(fp, db->partition_offset[i], SEEK_SET) == -1)
            return error(DCP_IOERROR, "failed to fseek");
    }
    db->profiles.curr_idx = 0;
    return DCP_DONE;
}

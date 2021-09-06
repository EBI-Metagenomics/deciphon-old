#include "dcp/pro_db.h"
#include "db.h"
#include "dcp/entry_dist.h"
#include "dcp/pro_cfg.h"
#include "dcp/pro_profile.h"
#include "dcp/rc.h"
#include "error.h"

struct dcp_pro_db
{
    struct dcp_db super;
    struct imm_amino amino;
    struct imm_nuclt nuclt;
    imm_float epsilon;
    enum entry_dist entry_dist;
    struct dcp_pro_prof prof;
};

static inline struct dcp_pro_db *new_db(void)
{
    struct dcp_pro_db *db = malloc(sizeof *db);
    db_init(&db->super, DCP_PROTEIN_PROFILE);
    dcp_pro_prof_init(&db->prof, &db->amino, &db->nuclt);
    return db;
}

static enum dcp_rc read_epsilon(cmp_ctx_t *ctx, unsigned float_bytes,
                                imm_float *epsilon)
{
    if (float_bytes == 4)
    {
        float e = 0;
        if (!cmp_read_float(ctx, &e))
            return error(DCP_IOERROR, "failed to read epsilon");
        *epsilon = (imm_float)e;
    }
    else
    {
        double e = 0;
        if (!cmp_read_double(ctx, &e))
            return error(DCP_IOERROR, "failed to read epsilon");
        *epsilon = (imm_float)e;
    }

    if (*epsilon < 0 || *epsilon > 1)
        return error(DCP_PARSEERROR, "invalid epsilon");

    return DCP_SUCCESS;
}

static enum dcp_rc write_epsilon(cmp_ctx_t *ctx, unsigned float_bytes,
                                 imm_float epsilon)
{
    if (float_bytes == 4)
    {
        float e = (float)epsilon;
        if (!cmp_write_float(ctx, e))
            return error(DCP_IOERROR, "failed to write epsilon");
    }
    else
    {
        double e = (double)epsilon;
        if (!cmp_write_double(ctx, e))
            return error(DCP_IOERROR, "failed to write epsilon");
    }

    return DCP_SUCCESS;
}

static enum dcp_rc read_entry_dist(cmp_ctx_t *ctx, enum dcp_entry_dist *edist)
{
    uint8_t val = 0;
    if (!cmp_read_u8(ctx, &val))
        return error(DCP_IOERROR, "failed to read entry distribution");
    *edist = val;
    return DCP_SUCCESS;
}

static enum dcp_rc write_entry_dist(cmp_ctx_t *ctx, enum dcp_entry_dist edist)
{
    if (!cmp_write_u8(ctx, (uint8_t)edist))
        return error(DCP_IOERROR, "failed to write entry distribution");
    return DCP_SUCCESS;
}

static enum dcp_rc read_nuclt(FILE *restrict fd, struct imm_nuclt *nuclt)
{
    if (imm_abc_read(&nuclt->super, fd))
        return error(DCP_IOERROR, "failed to read nuclt alphabet");
    return DCP_SUCCESS;
}

static enum dcp_rc write_nuclt(FILE *restrict fd, struct imm_nuclt const *nuclt)
{
    if (imm_abc_write(&nuclt->super, fd))
        return error(DCP_IOERROR, "failed to write nuclt alphabet");
    return DCP_SUCCESS;
}

static enum dcp_rc read_amino(FILE *restrict fd, struct imm_amino *amino)
{
    if (imm_abc_read(&amino->super, fd))
        return error(DCP_IOERROR, "failed to read amino alphabet");
    return DCP_SUCCESS;
}

static enum dcp_rc write_amino(FILE *restrict fd, struct imm_amino const *amino)
{
    if (imm_abc_write(&amino->super, fd))
        return error(DCP_IOERROR, "failed to write amino alphabet");
    return DCP_SUCCESS;
}

struct dcp_pro_db *dcp_pro_db_openr(FILE *restrict fd)
{
    struct dcp_pro_db *db = new_db();
    db_openr(&db->super, fd);

    cmp_ctx_t *ctx = &db->super.file.ctx;

    if (db_read_magic_number(&db->super)) goto cleanup;
    if (db_read_prof_type(&db->super)) goto cleanup;
    if (db_read_float_bytes(&db->super)) goto cleanup;
    if (read_epsilon(ctx, db->super.float_bytes, &db->epsilon)) goto cleanup;
    if (read_entry_dist(ctx, &db->entry_dist)) goto cleanup;
    if (read_nuclt(db->super.file.fd, &db->nuclt)) goto cleanup;
    if (read_amino(db->super.file.fd, &db->amino)) goto cleanup;
    if (db_read_nprofiles(&db->super)) goto cleanup;
    if (db_read_metadata(&db->super)) goto cleanup;

    assert(db->super.prof_typeid == DCP_PROTEIN_PROFILE);
    return db;

cleanup:
    free(db);
    return NULL;
}

struct dcp_pro_db *dcp_pro_db_openw(FILE *restrict fd, struct dcp_pro_cfg cfg)
{
    struct dcp_pro_db *db = new_db();
    db->amino = *cfg.amino;
    db->nuclt = *cfg.nuclt;
    if (db_openw(&db->super, fd)) goto cleanup;

    cmp_ctx_t *ctx = &db->super.file.ctx;

    if (db_write_magic_number(&db->super)) goto cleanup;
    if (db_write_prof_type(&db->super)) goto cleanup;
    if (db_write_float_size(&db->super)) goto cleanup;
    if (write_epsilon(ctx, db->super.float_bytes, db->epsilon)) goto cleanup;
    if (write_entry_dist(ctx, &db->entry_dist)) goto cleanup;
    if (write_nuclt(db->super.file.fd, &db->nuclt)) goto cleanup;
    if (write_amino(db->super.file.fd, &db->amino)) goto cleanup;

    return db;

cleanup:
    fclose(db->super.dp.fd);
    fclose(db->super.mt.file.fd);
    free(db);
    return NULL;
}

enum dcp_rc dcp_pro_db_close(struct dcp_pro_db *db)
{
    return db_close(&db->super);
}

struct imm_amino const *dcp_pro_db_amino(struct dcp_pro_db const *db)
{
    return &db->amino;
}

struct imm_nuclt const *dcp_pro_db_nuclt(struct dcp_pro_db const *db)
{
    return &db->nuclt;
}

enum dcp_rc dcp_pro_db_read(struct dcp_pro_db *db, struct dcp_pro_prof *prof)
{
    if (db_end(&db->super)) return error(DCP_RUNTIMEERROR, "end of profiles");

    prof->super.idx = db->super.profiles.curr_idx++;
    prof->super.mt = db_meta(&db->super, prof->super.idx);

    /* TODO: Call std_prof functions directly */
    return prof->super.vtable.read(&prof->super, db->super.file.fd);
}

enum dcp_rc dcp_pro_db_write(struct dcp_pro_db *db,
                             struct dcp_pro_prof const *prof)
{
    /* if ((rc = db_check_write_prof_ready(&db->super, &prof->super))) return
     * rc; */

    enum dcp_rc rc = DCP_SUCCESS;
    db_write_prof_meta(&db->super, &prof->super);

    /* TODO: Call std_prof functions directly */
    if ((rc = prof->super.vtable.write(&prof->super, db->super.dp.fd)))
        goto cleanup;

    /* db->profiles.size++; */

cleanup:
    return rc;
}

struct dcp_pro_prof *dcp_pro_db_profile(struct dcp_pro_db *db)
{
    return &db->prof;
}

unsigned dcp_pro_db_nprofiles(struct dcp_pro_db const *db)
{
    return db->super.profiles.size;
}

struct dcp_meta dcp_pro_db_meta(struct dcp_pro_db const *db, unsigned idx)
{
    return db_meta(&db->super, idx);
}

bool dcp_pro_db_end(struct dcp_pro_db const *db) { return db_end(&db->super); }

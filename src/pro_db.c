#include "dcp/pro_db.h"
#include "db.h"
#include "dcp/entry_dist.h"
#include "dcp/pro_cfg.h"
#include "dcp/pro_prof.h"
#include "dcp/rc.h"
#include "error.h"
#include "pro_prof.h"
#include "third-party/cmp.h"

struct dcp_pro_db
{
    struct dcp_db super;
    struct imm_amino amino;
    struct imm_nuclt nuclt;
    struct dcp_pro_prof prof;
};

static enum dcp_rc read_epsilon(struct dcp_cmp_ctx *ctx, unsigned float_bytes,
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

static enum dcp_rc write_epsilon(struct dcp_cmp_ctx *ctx, unsigned float_bytes,
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

static enum dcp_rc read_entry_dist(struct dcp_cmp_ctx *ctx,
                                   enum dcp_entry_dist *edist)
{
    uint8_t val = 0;
    if (!cmp_read_u8(ctx, &val))
        return error(DCP_IOERROR, "failed to read entry distribution");
    *edist = val;
    return DCP_SUCCESS;
}

static enum dcp_rc write_entry_dist(struct dcp_cmp_ctx *ctx,
                                    enum dcp_entry_dist edist)
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
    struct dcp_pro_db *db = malloc(sizeof *db);
    if (!db)
    {
        error(DCP_OUTOFMEM, "failed to alloc db");
        return NULL;
    }
    db_init(&db->super, DCP_PROTEIN_PROFILE);
    dcp_pro_prof_init(&db->prof, &db->amino, &db->nuclt, DCP_PRO_CFG_DEFAULT);
    db_openr(&db->super, fd);

    struct dcp_cmp_ctx *ctx = &db->super.file.ctx;
    imm_float *epsilon = &db->prof.cfg.epsilon;

    if (db_read_magic_number(&db->super)) goto cleanup;
    if (db_read_prof_type(&db->super)) goto cleanup;
    if (db_read_float_size(&db->super)) goto cleanup;
    if (read_entry_dist(ctx, &db->prof.cfg.entry_dist)) goto cleanup;
    if (read_epsilon(ctx, db->super.float_size, epsilon)) goto cleanup;
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

struct dcp_pro_db *dcp_pro_db_openw(FILE *restrict fd,
                                    struct imm_amino const *amino,
                                    struct imm_nuclt const *nuclt,
                                    struct dcp_pro_cfg cfg)
{
    struct dcp_pro_db *db = malloc(sizeof *db);
    if (!db)
    {
        error(DCP_OUTOFMEM, "failed to alloc db");
        return NULL;
    }
    db->amino = *amino;
    db->nuclt = *nuclt;
    db_init(&db->super, DCP_PROTEIN_PROFILE);
    dcp_pro_prof_init(&db->prof, &db->amino, &db->nuclt, cfg);
    if (db_openw(&db->super, fd)) goto cleanup;

    struct dcp_cmp_ctx *ctx = &db->super.file.ctx;
    unsigned float_size = db->super.float_size;

    if (db_write_magic_number(&db->super)) goto cleanup;
    if (db_write_prof_type(&db->super)) goto cleanup;
    if (db_write_float_size(&db->super)) goto cleanup;
    if (write_entry_dist(ctx, db->prof.cfg.entry_dist)) goto cleanup;
    if (write_epsilon(ctx, float_size, db->prof.cfg.epsilon)) goto cleanup;
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

struct dcp_pro_cfg dcp_pro_db_cfg(struct dcp_pro_db const *db)
{
    return db->prof.cfg;
}

enum dcp_rc dcp_pro_db_read(struct dcp_pro_db *db, struct dcp_pro_prof *prof)
{
    if (db_end(&db->super)) return error(DCP_RUNTIMEERROR, "end of profiles");
    prof->super.idx = db->super.profiles.curr_idx++;
    prof->super.mt = db_meta(&db->super, prof->super.idx);
    return pro_prof_read(prof, db->super.file.fd);
}

enum dcp_rc dcp_pro_db_write(struct dcp_pro_db *db,
                             struct dcp_pro_prof const *prof)
{
    /* if ((rc = db_check_write_prof_ready(&db->super, &prof->super))) return
     * rc; */
    enum dcp_rc rc = DCP_SUCCESS;
    if ((rc = db_write_prof_meta(&db->super, &prof->super))) return rc;
    if ((rc = pro_prof_write(prof, db->super.dp.fd))) return rc;
    db->super.profiles.size++;
    return rc;
}

struct dcp_pro_prof *dcp_pro_db_profile(struct dcp_pro_db *db)
{
    return &db->prof;
}

struct dcp_db *dcp_pro_db_super(struct dcp_pro_db *db) { return &db->super; }

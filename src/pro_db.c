#include "dcp/pro_db.h"
#include "db.h"
#include "dcp/entry_dist.h"
#include "dcp/pro_cfg.h"
#include "dcp/pro_prof.h"
#include "dcp/rc.h"
#include "error.h"
#include "pro_prof.h"
#include "third-party/cmp.h"

struct dcp_pro_db const dcp_pro_db_default = {0};

static enum dcp_rc read_epsilon(struct dcp_cmp *cmp, unsigned float_bytes,
                                imm_float *epsilon)
{
    if (float_bytes == 4)
    {
        float e = 0;
        if (!cmp_read_float(cmp, &e))
            return error(DCP_IOERROR, "failed to read epsilon");
        *epsilon = (imm_float)e;
    }
    else
    {
        double e = 0;
        if (!cmp_read_double(cmp, &e))
            return error(DCP_IOERROR, "failed to read epsilon");
        *epsilon = (imm_float)e;
    }

    if (*epsilon < 0 || *epsilon > 1)
        return error(DCP_PARSEERROR, "invalid epsilon");

    return DCP_DONE;
}

static enum dcp_rc write_epsilon(struct dcp_cmp *cmp, unsigned float_bytes,
                                 imm_float epsilon)
{
    if (float_bytes == 4)
    {
        float e = (float)epsilon;
        if (!cmp_write_float(cmp, e))
            return error(DCP_IOERROR, "failed to write epsilon");
    }
    else
    {
        double e = (double)epsilon;
        if (!cmp_write_double(cmp, e))
            return error(DCP_IOERROR, "failed to write epsilon");
    }

    return DCP_DONE;
}

static enum dcp_rc read_entry_dist(struct dcp_cmp *cmp,
                                   enum dcp_entry_dist *edist)
{
    uint8_t val = 0;
    if (!cmp_read_u8(cmp, &val))
        return error(DCP_IOERROR, "failed to read entry distribution");
    *edist = val;
    return DCP_DONE;
}

static enum dcp_rc write_entry_dist(struct dcp_cmp *cmp,
                                    enum dcp_entry_dist edist)
{
    if (!cmp_write_u8(cmp, (uint8_t)edist))
        return error(DCP_IOERROR, "failed to write entry distribution");
    return DCP_DONE;
}

static enum dcp_rc read_nuclt(FILE *restrict fd, struct imm_nuclt *nuclt)
{
    if (imm_abc_read(&nuclt->super, fd))
        return error(DCP_IOERROR, "failed to read nuclt alphabet");
    return DCP_DONE;
}

static enum dcp_rc write_nuclt(FILE *restrict fd, struct imm_nuclt const *nuclt)
{
    if (imm_abc_write(&nuclt->super, fd))
        return error(DCP_IOERROR, "failed to write nuclt alphabet");
    return DCP_DONE;
}

static enum dcp_rc read_amino(FILE *restrict fd, struct imm_amino *amino)
{
    if (imm_abc_read(&amino->super, fd))
        return error(DCP_IOERROR, "failed to read amino alphabet");
    return DCP_DONE;
}

static enum dcp_rc write_amino(FILE *restrict fd, struct imm_amino const *amino)
{
    if (imm_abc_write(&amino->super, fd))
        return error(DCP_IOERROR, "failed to write amino alphabet");
    return DCP_DONE;
}

static void pro_db_init(struct dcp_pro_db *db)
{
    db_init(&db->super, DCP_PRO_PROFILE);
    db->amino = imm_amino_empty;
    db->nuclt = imm_nuclt_empty;
    db->code = imm_nuclt_code_empty;
    db->code.nuclt = &db->nuclt;
    dcp_pro_prof_init(&db->prof, &db->amino, &db->code, DCP_PRO_CFG_DEFAULT);
}

void dcp_pro_db_setup_multi_readers(struct dcp_pro_db *db, unsigned nfiles,
                                    FILE *fp[])
{
}

enum dcp_rc dcp_pro_db_openr(struct dcp_pro_db *db, FILE *restrict fd)
{
    pro_db_init(db);
    db_openr(&db->super, fd);

    struct dcp_cmp *cmp = &db->super.file.cmp[0];
    imm_float *epsilon = &db->prof.cfg.epsilon;

    enum dcp_rc rc = DCP_DONE;
    if ((rc = db_read_magic_number(&db->super))) return rc;
    if ((rc = db_read_prof_type(&db->super))) return rc;
    if ((rc = db_read_float_size(&db->super))) return rc;
    if ((rc = read_entry_dist(cmp, &db->prof.cfg.entry_dist))) return rc;
    if ((rc = read_epsilon(cmp, db->super.float_size, epsilon))) return rc;
    if ((rc = read_nuclt(fd, &db->nuclt))) return rc;
    if ((rc = read_amino(fd, &db->amino))) return rc;
    if ((rc = db_read_nprofiles(&db->super))) return rc;
    if ((rc = db_read_metadata(&db->super))) return rc;

    imm_code_init(&db->code.super, imm_super(&db->nuclt));
    assert(db->super.prof_typeid == DCP_PRO_PROFILE);
    return db_record_prof_offset(&db->super);
}

enum dcp_rc dcp_pro_db_openw(struct dcp_pro_db *db, FILE *restrict fd,
                             struct imm_amino const *amino,
                             struct imm_nuclt const *nuclt,
                             struct dcp_pro_cfg cfg)
{
    pro_db_init(db);
    db->amino = *amino;
    db->nuclt = *nuclt;
    imm_nuclt_code_init(&db->code, &db->nuclt);

    struct dcp_cmp *cmp = &db->super.file.cmp[0];
    unsigned float_size = db->super.float_size;
    imm_float epsilon = db->prof.cfg.epsilon;

    enum dcp_rc rc = DCP_DONE;
    if ((rc = db_openw(&db->super, fd))) goto cleanup;
    if ((rc = db_write_magic_number(&db->super))) goto cleanup;
    if ((rc = db_write_prof_type(&db->super))) goto cleanup;
    if ((rc = db_write_float_size(&db->super))) goto cleanup;
    if ((rc = write_entry_dist(cmp, db->prof.cfg.entry_dist))) goto cleanup;
    if ((rc = write_epsilon(cmp, float_size, epsilon))) goto cleanup;
    if ((rc = write_nuclt(fd, &db->nuclt))) goto cleanup;
    if ((rc = write_amino(fd, &db->amino))) goto cleanup;

    return rc;

cleanup:
    dcp_cmp_close(&db->super.mt.file.cmp);
    dcp_cmp_close(&db->super.dp.cmp);
    return rc;
}

enum dcp_rc dcp_pro_db_close(struct dcp_pro_db *db)
{
    enum dcp_rc rc = db_close(&db->super);
    dcp_pro_prof_del(&db->prof);
    return rc;
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
    if (db_end(&db->super)) return error(DCP_FAIL, "end of profiles");
    prof->super.idx = db->super.profiles.curr_idx++;
    prof->super.mt = db_meta(&db->super, prof->super.idx);
    return pro_prof_read(prof, &db->super.file.cmp[0]);
}

enum dcp_rc dcp_pro_db_write(struct dcp_pro_db *db,
                             struct dcp_pro_prof const *prof)
{
    /* TODO: db_check_write_prof_ready(&db->super, &prof->super) */
    enum dcp_rc rc = DCP_DONE;
    if ((rc = db_write_prof_meta(&db->super, &prof->super))) return rc;
    if ((rc = pro_prof_write(prof, &db->super.dp.cmp))) return rc;
    db->super.profiles.size++;
    return rc;
}

struct dcp_pro_prof *dcp_pro_db_profile(struct dcp_pro_db *db)
{
    return &db->prof;
}

struct dcp_db *dcp_pro_db_super(struct dcp_pro_db *db) { return &db->super; }

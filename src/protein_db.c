#include "protein_db.h"
#include "cmp/cmp.h"
#include "db.h"
#include "db_types.h"
#include "entry_dist.h"
#include "logger.h"
#include "protein_cfg.h"
#include "protein_profile.h"
#include "rc.h"
#include "xmath.h"

static enum rc close(struct db *db) { return __db_close(db); }

static struct imm_abc const *abc(struct db const *db)
{
    struct protein_db const *p = (struct protein_db *)db;
    return &p->nuclt.super;
}

static struct db_vtable vtable = {DB_PROTEIN, close, abc};

static enum rc read_epsilon(struct cmp_ctx_s *cmp, unsigned float_bytes,
                            imm_float *epsilon)
{
    if (float_bytes == 4)
    {
        float e = 0;
        if (!cmp_read_float(cmp, &e))
            return error(RC_IOERROR, "failed to read epsilon");
        *epsilon = (imm_float)e;
    }
    else
    {
        double e = 0;
        if (!cmp_read_double(cmp, &e))
            return error(RC_IOERROR, "failed to read epsilon");
        *epsilon = (imm_float)e;
    }

    if (*epsilon < 0 || *epsilon > 1)
        return error(RC_PARSEERROR, "invalid epsilon");

    return RC_DONE;
}

static enum rc write_epsilon(struct cmp_ctx_s *cmp, unsigned float_bytes,
                             imm_float epsilon)
{
    if (float_bytes == 4)
    {
        float e = (float)epsilon;
        if (!cmp_write_float(cmp, e))
            return error(RC_IOERROR, "failed to write epsilon");
    }
    else
    {
        double e = (double)epsilon;
        if (!cmp_write_double(cmp, e))
            return error(RC_IOERROR, "failed to write epsilon");
    }

    return RC_DONE;
}

static enum rc read_entry_dist(struct cmp_ctx_s *cmp, enum entry_dist *edist)
{
    uint8_t val = 0;
    if (!cmp_read_u8(cmp, &val))
        return error(RC_IOERROR, "failed to read entry distribution");
    *edist = val;
    return RC_DONE;
}

static enum rc write_entry_dist(struct cmp_ctx_s *cmp, enum entry_dist edist)
{
    if (!cmp_write_u8(cmp, (uint8_t)edist))
        return error(RC_IOERROR, "failed to write entry distribution");
    return RC_DONE;
}

static enum rc read_nuclt(FILE *restrict fd, struct imm_nuclt *nuclt)
{
    if (imm_abc_read(&nuclt->super, fd))
        return error(RC_IOERROR, "failed to read nuclt alphabet");
    return RC_DONE;
}

static enum rc write_nuclt(FILE *restrict fd, struct imm_nuclt const *nuclt)
{
    if (imm_abc_write(&nuclt->super, fd))
        return error(RC_IOERROR, "failed to write nuclt alphabet");
    return RC_DONE;
}

static enum rc read_amino(FILE *restrict fd, struct imm_amino *amino)
{
    if (imm_abc_read(&amino->super, fd))
        return error(RC_IOERROR, "failed to read amino alphabet");
    return RC_DONE;
}

static enum rc write_amino(FILE *restrict fd, struct imm_amino const *amino)
{
    if (imm_abc_write(&amino->super, fd))
        return error(RC_IOERROR, "failed to write amino alphabet");
    return RC_DONE;
}

static void protein_db_init(struct protein_db *db)
{
    db_init(&db->super, vtable);
    db->amino = imm_amino_empty;
    db->nuclt = imm_nuclt_empty;
    db->code = imm_nuclt_code_empty;
    db->code.nuclt = &db->nuclt;
}

enum rc protein_db_openr(struct protein_db *db, FILE *restrict fp)
{
    protein_db_init(db);
    db_openr(&db->super, fp);

    struct cmp_ctx_s *cmp = &db->super.file.cmp;
    imm_float *epsilon = &db->cfg.epsilon;

    enum rc rc = RC_DONE;
    if ((rc = db_read_magic_number(&db->super))) return rc;
    if ((rc = db_read_profile_typeid(&db->super))) return rc;
    if ((rc = db_read_float_size(&db->super))) return rc;
    if ((rc = read_entry_dist(cmp, &db->cfg.entry_dist))) return rc;
    if ((rc = read_epsilon(cmp, db->super.float_size, epsilon))) return rc;
    if ((rc = read_nuclt(fp, &db->nuclt))) return rc;
    if ((rc = read_amino(fp, &db->amino))) return rc;
    if ((rc = db_read_nprofiles(&db->super))) return rc;
    if ((rc = db_read_metadata(&db->super))) return rc;

    imm_code_init(&db->code.super, imm_super(&db->nuclt));
    if (db->super.vtable.typeid != DB_PROTEIN)
        return error(RC_PARSEERROR, "wrong typeid");

    return db_set_metadata_end(&db->super);
}

static void cleanup(struct protein_db *db)
{
    fclose(cmp_file(&db->super.mt.file.cmp));
    fclose(cmp_file(&db->super.dp.cmp));
}

enum rc protein_db_openw(struct protein_db *db, FILE *restrict fd,
                         struct imm_amino const *amino,
                         struct imm_nuclt const *nuclt, struct protein_cfg cfg)
{
    protein_db_init(db);
    db->amino = *amino;
    db->nuclt = *nuclt;
    db->cfg = cfg;
    imm_nuclt_code_init(&db->code, &db->nuclt);

    struct cmp_ctx_s *cmp = &db->super.file.cmp;
    unsigned float_size = db->super.float_size;
    imm_float epsilon = db->cfg.epsilon;

    enum rc rc = RC_DONE;
    if ((rc = db_openw(&db->super, fd))) goto cleanup;
    if ((rc = db_write_magic_number(&db->super))) goto cleanup;
    if ((rc = db_write_prof_type(&db->super))) goto cleanup;
    if ((rc = db_write_float_size(&db->super))) goto cleanup;
    if ((rc = write_entry_dist(cmp, db->cfg.entry_dist))) goto cleanup;
    if ((rc = write_epsilon(cmp, float_size, epsilon))) goto cleanup;
    if ((rc = write_nuclt(fd, &db->nuclt))) goto cleanup;
    if ((rc = write_amino(fd, &db->amino))) goto cleanup;

    return rc;

cleanup:
    cleanup(db);
    return rc;
}

struct imm_amino const *protein_db_amino(struct protein_db const *db)
{
    return &db->amino;
}

struct imm_nuclt const *protein_db_nuclt(struct protein_db const *db)
{
    return &db->nuclt;
}

struct protein_cfg protein_db_cfg(struct protein_db const *db)
{
    return db->cfg;
}

enum rc protein_db_write(struct protein_db *db,
                         struct protein_profile const *prof, struct metadata mt)
{
    /* TODO: db_check_write_prof_ready(&db->super, &prof->super) */
    enum rc rc = RC_DONE;
    if ((rc = db_write_profile_metadata(&db->super, mt))) return rc;
    if ((rc = protein_profile_write(prof, &db->super.dp.cmp))) return rc;
    db->super.profiles.size++;
    return rc;
}

struct db *protein_db_super(struct protein_db *db) { return &db->super; }

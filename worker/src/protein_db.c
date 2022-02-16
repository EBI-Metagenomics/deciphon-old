#include "protein_db.h"
#include "cmp/cmp.h"
#include "common/logger.h"
#include "common/rc.h"
#include "common/xmath.h"
#include "db.h"
#include "db_types.h"
#include "entry_dist.h"
#include "js.h"
#include "profile_types.h"
#include "protein_cfg.h"
#include "protein_profile.h"

static struct imm_abc const *abc(struct db const *db)
{
    struct protein_db const *p = (struct protein_db *)db;
    return &p->nuclt.super;
}

static enum rc write_profile(struct cmp_ctx_s *dst, struct profile const *prof)
{
    /* TODO: db_check_write_prof_ready(&db->super, &prof->super) */
    struct protein_profile const *p = (struct protein_profile const *)prof;
    return protein_profile_write(p, dst);
}

static struct db_vtable vtable = {DB_PROTEIN, abc, write_profile, 8};

static enum rc read_epsilon(struct cmp_ctx_s *cmp, unsigned float_bytes,
                            imm_float *epsilon)
{
    if (!JS_XPEC_STR(cmp, "epsilon")) eio("skip key");
    double e = 0;
    if (!cmp_read_decimal(cmp, &e)) return eio("read epsilon");
    *epsilon = (imm_float)e;

    if (*epsilon < 0 || *epsilon > 1)
        return error(RC_EPARSE, "invalid epsilon");

    return RC_DONE;
}

static enum rc write_epsilon(struct cmp_ctx_s *cmp, unsigned float_bytes,
                             imm_float epsilon)
{
    if (!JS_WRITE_STR(cmp, "epsilon")) eio("write epsilon key");

    assert(float_bytes == 4 || float_bytes == 8);
    double e = 0;
    if (float_bytes == 4)
        e = (float)epsilon;
    else
        e = (double)epsilon;
    if (!cmp_write_decimal(cmp, e)) return eio("write epsilon");
    return RC_DONE;
}

static enum rc read_entry_dist(struct cmp_ctx_s *cmp, enum entry_dist *edist)
{
    if (!JS_XPEC_STR(cmp, "entry_dist")) eio("skip key");
    uint64_t val = 0;
    if (!cmp_read_uinteger(cmp, &val)) return eio("read entry_dist");
    *edist = (enum entry_dist)val;
    return RC_DONE;
}

static enum rc write_entry_dist(struct cmp_ctx_s *cmp, enum entry_dist edist)
{
    if (!JS_WRITE_STR(cmp, "entry_dist")) eio("write entry_dist key");
    if (!cmp_write_uinteger(cmp, (uint64_t)edist))
        return eio("write entry distribution");
    return RC_DONE;
}

static enum rc read_nuclt(struct cmp_ctx_s *cmp, struct imm_nuclt *nuclt)
{
    if (!JS_XPEC_STR(cmp, "abc")) eio("skip abc key");
    if (imm_abc_read_cmp(&nuclt->super, cmp)) return eio("read nuclt abc");
    return RC_DONE;
}

static enum rc write_nuclt(struct cmp_ctx_s *cmp, struct imm_nuclt const *nuclt)
{
    if (!JS_WRITE_STR(cmp, "abc")) eio("write abc key");
    if (imm_abc_write(&nuclt->super, cmp_file(cmp)))
        return eio("write nuclt abc");
    return RC_DONE;
}

static enum rc read_amino(struct cmp_ctx_s *cmp, struct imm_amino *amino)
{
    if (!JS_XPEC_STR(cmp, "amino")) eio("skip amino key");
    if (imm_abc_read_cmp(&amino->super, cmp)) return eio("read amino abc");
    return RC_DONE;
}

static enum rc write_amino(struct cmp_ctx_s *cmp, struct imm_amino const *amino)
{
    if (!JS_WRITE_STR(cmp, "amino")) eio("write amino key");
    if (imm_abc_write(&amino->super, cmp_file(cmp)))
        return eio("write amino abc");
    return RC_DONE;
}

static void protein_db_init(struct protein_db *db)
{
    db_init(&db->super, vtable);
    db->super.profile_typeid = PROFILE_PROTEIN;
    db->amino = imm_amino_empty;
    db->nuclt = imm_nuclt_empty;
    db->code = imm_nuclt_code_empty;
    db->code.nuclt = &db->nuclt;
}

enum rc protein_db_openr(struct protein_db *db, FILE *fp)
{
    protein_db_init(db);
    db_openr(&db->super, fp);

    struct cmp_ctx_s *cmp = &db->super.file.cmp;
    imm_float *epsilon = &db->cfg.epsilon;

    enum rc rc = RC_DONE;
    JS_XPEC_STR(cmp, "header");

    uint32_t size = 0;
    cmp_read_map(&db->super.file.cmp, &size);
    assert(size == db->super.vtable.header_size);

    if ((rc = db_read_magic_number(&db->super))) return rc;
    if ((rc = db_read_profile_typeid(&db->super))) return rc;
    if ((rc = db_read_float_size(&db->super))) return rc;
    if ((rc = read_entry_dist(cmp, &db->cfg.entry_dist))) return rc;
    if ((rc = read_epsilon(cmp, db->super.float_size, epsilon))) return rc;
    if ((rc = read_nuclt(cmp, &db->nuclt))) return rc;
    if ((rc = read_amino(cmp, &db->amino))) return rc;
    if ((rc = db_read_profile_sizes(&db->super))) return rc;
    if ((rc = db_read_metadata(&db->super))) return rc;

    imm_code_init(&db->code.super, imm_super(&db->nuclt));
    if (db->super.vtable.typeid != DB_PROTEIN)
        return error(RC_EPARSE, "wrong typeid");

    return db_set_metadata_end(&db->super);
}

enum rc protein_db_openw(struct protein_db *db, FILE *fp,
                         struct imm_amino const *amino,
                         struct imm_nuclt const *nuclt, struct protein_cfg cfg)
{
    protein_db_init(db);
    db->amino = *amino;
    db->nuclt = *nuclt;
    db->cfg = cfg;
    imm_nuclt_code_init(&db->code, &db->nuclt);

    struct cmp_ctx_s *hdr = &db->super.tmp.hdr;
    unsigned float_size = db->super.float_size;
    imm_float epsilon = db->cfg.epsilon;

    enum rc rc = RC_DONE;
    if ((rc = db_openw(&db->super, fp))) goto cleanup;
    if ((rc = db_write_magic_number(&db->super))) goto cleanup;
    if ((rc = db_write_profile_typeid(&db->super))) goto cleanup;
    if ((rc = db_write_float_size(&db->super))) goto cleanup;
    if ((rc = write_entry_dist(hdr, db->cfg.entry_dist))) goto cleanup;
    if ((rc = write_epsilon(hdr, float_size, epsilon))) goto cleanup;
    if ((rc = write_nuclt(hdr, &db->nuclt))) goto cleanup;
    if ((rc = write_amino(hdr, &db->amino))) goto cleanup;

    return rc;

cleanup:
    db_cleanup((struct db *)&db);
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

struct db *protein_db_super(struct protein_db *db) { return &db->super; }

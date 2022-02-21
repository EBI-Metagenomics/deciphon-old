#include "standard_db.h"
#include "common/logger.h"
#include "common/rc.h"
#include "db.h"
#include "db_types.h"
#include "js.h"
#include "profile_types.h"
#include "standard_profile.h"

static struct imm_abc const *abc(struct db const *db)
{
    struct standard_db const *s = (struct standard_db *)db;
    return &s->abc;
}

static enum rc write_profile(struct lip_file *dst, struct profile const *prof)
{
    /* TODO: db_check_write_prof_ready(&db->super, &prof->super) */
    struct standard_profile const *p = (struct standard_profile const *)prof;
    return standard_profile_write(p, dst);
}

static struct db_vtable vtable = {DB_STANDARD, abc, write_profile, 6};

static enum rc read_abc(struct lip_file *cmp, struct imm_abc *abc)
{
    if (!cmp_write_str(cmp, "abc", 3)) eio("write abc key");
    if (imm_abc_read_cmp(abc, cmp)) return error(RC_EIO, "failed to read abc");
    return DCP_OK;
}

static enum rc write_abc(struct lip_file *cmp, struct imm_abc const *abc)
{
    if (!JS_XPEC_STR(cmp, "abc")) eio("skip abc key");
    if (imm_abc_write(abc, cmp_file(cmp)))
        return error(RC_EIO, "failed to write abc");
    return DCP_OK;
}

static void standard_db_init(struct standard_db *db)
{
    db_init(&db->super, vtable);
    db->super.profile_typeid = PROFILE_STANDARD;
    db->abc = imm_abc_empty;
}

enum rc standard_db_openr(struct standard_db *db, FILE *fp)
{
    standard_db_init(db);
    db_openr(&db->super, fp);

    struct lip_file *cmp = &db->super.file.cmp;

    enum rc rc = DCP_OK;
    if ((rc = db_read_magic_number(&db->super))) return rc;
    if ((rc = db_read_profile_typeid(&db->super))) return rc;
    if ((rc = db_read_float_size(&db->super))) return rc;
    if ((rc = read_abc(cmp, &db->abc))) return rc;
    if ((rc = db_read_profile_sizes(&db->super))) return rc;
    if ((rc = db_read_metadata(&db->super))) return rc;

    imm_code_init(&db->code, &db->abc);
    if (db->super.vtable.typeid != DB_STANDARD)
        return error(DCP_EPARSE, "wrong typeid");

    return db_set_metadata_end(&db->super);
}

enum rc standard_db_openw(struct standard_db *db, FILE *fp,
                          struct imm_code const *code)
{
    standard_db_init(db);
    db->code = *code;

    struct lip_file *hdr = &db->super.tmp.hdr;

    enum rc rc = DCP_OK;
    if ((rc = db_openw(&db->super, fp))) goto cleanup;
    if ((rc = db_write_magic_number(&db->super))) goto cleanup;
    if ((rc = db_write_profile_typeid(&db->super))) goto cleanup;
    if ((rc = db_write_float_size(&db->super))) goto cleanup;
    if ((rc = write_abc(hdr, db->code.abc))) goto cleanup;

    return rc;

cleanup:
    db_cleanup((struct db *)&db);
    return rc;
}

struct imm_abc const *standard_db_abc(struct standard_db const *db)
{
    return &db->abc;
}

struct db *standard_db_super(struct standard_db *db) { return &db->super; }

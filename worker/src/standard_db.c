#include "standard_db.h"
#include "common/logger.h"
#include "common/rc.h"
#include "db.h"
#include "db_types.h"
#include "standard_profile.h"

static enum rc close(struct db *db) { return __db_close(db); }

static struct imm_abc const *abc(struct db const *db)
{
    struct standard_db const *s = (struct standard_db *)db;
    return &s->abc;
}

static struct db_vtable vtable = {DB_STANDARD, close, abc};

static enum rc read_abc(FILE *fp, struct imm_abc *abc)
{
    if (imm_abc_read(abc, fp)) return error(RC_EIO, "failed to read alphabet");
    return RC_DONE;
}

static enum rc write_abc(FILE *fp, struct imm_abc const *abc)
{
    if (imm_abc_write(abc, fp))
        return error(RC_EIO, "failed to write alphabet");
    return RC_DONE;
}

static void standard_db_init(struct standard_db *db)
{
    db_init(&db->super, vtable);
    db->abc = imm_abc_empty;
}

enum rc standard_db_openr(struct standard_db *db, FILE *fp)
{
    standard_db_init(db);
    db_openr(&db->super, fp);

    enum rc rc = RC_DONE;
    if ((rc = db_read_magic_number(&db->super))) return rc;
    if ((rc = db_read_profile_typeid(&db->super))) return rc;
    if ((rc = db_read_float_size(&db->super))) return rc;
    if ((rc = read_abc(fp, &db->abc))) return rc;
    if ((rc = db_read_nprofiles(&db->super))) return rc;
    if ((rc = db_read_metadata(&db->super))) return rc;

    imm_code_init(&db->code, &db->abc);
    if (db->super.vtable.typeid != DB_STANDARD)
        return error(RC_EPARSE, "wrong typeid");

    return db_set_metadata_end(&db->super);
}

static void cleanup(struct standard_db *db)
{
    fclose(cmp_file(&db->super.mt.file.cmp));
    fclose(cmp_file(&db->super.dp.cmp));
}

enum rc standard_db_openw(struct standard_db *db, FILE *fp,
                          struct imm_code const *code)
{
    standard_db_init(db);
    db->code = *code;

    enum rc rc = RC_DONE;
    if ((rc = db_openw(&db->super, fp))) goto cleanup;
    if ((rc = db_write_magic_number(&db->super))) goto cleanup;
    if ((rc = db_write_prof_type(&db->super))) goto cleanup;
    if ((rc = db_write_float_size(&db->super))) goto cleanup;
    if ((rc = write_abc(fp, db->code.abc))) goto cleanup;

    return rc;

cleanup:
    cleanup(db);
    return rc;
}

struct imm_abc const *standard_db_abc(struct standard_db const *db)
{
    return &db->abc;
}

enum rc standard_db_write(struct standard_db *db,
                          struct standard_profile const *prof,
                          struct metadata mt)
{
    /* TODO: db_check_write_prof_ready(&db->super, &prof->super) */
    enum rc rc = RC_DONE;
    if ((rc = db_write_profile_metadata(&db->super, mt))) return rc;
    if ((rc = standard_profile_write(prof, cmp_file(&db->super.dp.cmp))))
        return rc;
    db->super.profiles.size++;
    return rc;
}

struct db *standard_db_super(struct standard_db *db) { return &db->super; }

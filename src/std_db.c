#include "std_db.h"
#include "db.h"
#include "logger.h"
#include "rc.h"
#include "std_prof.h"
#include "xcmp.h"

static enum rc read_abc(FILE *restrict fd, struct imm_abc *abc)
{
    if (imm_abc_read(abc, fd))
        return error(IOERROR, "failed to read alphabet");
    return DONE;
}

static enum rc write_abc(FILE *restrict fd, struct imm_abc const *abc)
{
    if (imm_abc_write(abc, fd))
        return error(IOERROR, "failed to write alphabet");
    return DONE;
}

void dcp_std_db_init(struct dcp_std_db *db)
{
    db_init(&db->super, DCP_STD_PROFILE);
    db->abc = imm_abc_empty;
    std_prof_init(&db->prof, &db->code);
}

enum rc dcp_std_db_openr(struct dcp_std_db *db, FILE *restrict fd)
{
    db_openr(&db->super, fd);

    enum rc rc = DONE;
    if ((rc = db_read_magic_number(&db->super))) return rc;
    if ((rc = db_read_prof_type(&db->super))) return rc;
    if ((rc = db_read_float_size(&db->super))) return rc;
    if ((rc = read_abc(fd, &db->abc))) return rc;
    if ((rc = db_read_nprofiles(&db->super))) return rc;
    if ((rc = db_read_metadata(&db->super))) return rc;

    imm_code_init(&db->code, &db->abc);
    assert(db->super.prof_typeid == DCP_STD_PROFILE);
    return rc;
}

enum rc dcp_std_db_openw(struct dcp_std_db *db, FILE *restrict fd,
                             struct imm_code const *code)
{
    db->code = *code;

    enum rc rc = DONE;
    if ((rc = db_openw(&db->super, fd))) goto cleanup;
    if ((rc = db_write_magic_number(&db->super))) goto cleanup;
    if ((rc = db_write_prof_type(&db->super))) goto cleanup;
    if ((rc = db_write_float_size(&db->super))) goto cleanup;
    if ((rc = write_abc(fd, db->code.abc))) goto cleanup;

    return rc;

cleanup:
    xcmp_close(&db->super.mt.file.cmp);
    xcmp_close(&db->super.dp.cmp);
    return rc;
}

enum rc dcp_std_db_close(struct dcp_std_db *db)
{
    enum rc rc = db_close(&db->super);
    dcp_std_prof_del(&db->prof);
    return rc;
}

struct imm_abc const *dcp_std_db_abc(struct dcp_std_db const *db)
{
    return &db->abc;
}

enum rc dcp_std_db_read(struct dcp_std_db *db, struct std_prof *prof)
{
    if (db_end(&db->super)) return error(FAIL, "end of profiles");
    prof->super.idx = db->super.profiles.curr_idx++;
    prof->super.mt = db_meta(&db->super, prof->super.idx);
    return std_prof_read(prof, xcmp_fp(&db->super.file.cmp[0]));
}

enum rc dcp_std_db_write(struct dcp_std_db *db,
                             struct std_prof const *prof)
{
    /* TODO: db_check_write_prof_ready(&db->super, &prof->super) */
    enum rc rc = DONE;
    if ((rc = db_write_prof_meta(&db->super, &prof->super))) return rc;
    if ((rc = std_prof_write(prof, xcmp_fp(&db->super.dp.cmp)))) return rc;
    db->super.profiles.size++;
    return rc;
}

struct std_prof *dcp_std_db_profile(struct dcp_std_db *db)
{
    return &db->prof;
}

struct dcp_db *dcp_std_db_super(struct dcp_std_db *db) { return &db->super; }

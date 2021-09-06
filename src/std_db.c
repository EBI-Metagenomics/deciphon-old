#include "dcp/std_db.h"
#include "db.h"
#include "dcp/rc.h"
#include "dcp/std_cfg.h"
#include "dcp/std_profile.h"
#include "error.h"
#include "std_prof.h"

struct dcp_std_db
{
    struct dcp_db super;
    struct imm_abc abc;
    struct dcp_std_prof prof;
};

static enum dcp_rc read_abc(FILE *restrict fd, struct imm_abc *abc)
{
    if (imm_abc_read(abc, fd))
        return error(DCP_IOERROR, "failed to read alphabet");
    return DCP_SUCCESS;
}

static enum dcp_rc write_abc(FILE *restrict fd, struct imm_abc const *abc)
{
    if (imm_abc_write(abc, fd))
        return error(DCP_IOERROR, "failed to write alphabet");
    return DCP_SUCCESS;
}

struct dcp_std_db *dcp_std_db_openr(FILE *restrict fd)
{
    struct dcp_std_db *db = malloc(sizeof *db);
    if (!db)
    {
        error(DCP_OUTOFMEM, "failed to alloc db");
        return NULL;
    }
    db_init(&db->super, DCP_STD_PROFILE);
    db_openr(&db->super, fd);

    if (db_read_magic_number(&db->super)) goto cleanup;
    if (db_read_prof_type(&db->super)) goto cleanup;
    if (db_read_float_size(&db->super)) goto cleanup;
    if (read_abc(db->super.file.fd, &db->abc)) goto cleanup;
    if (db_read_nprofiles(&db->super)) goto cleanup;
    if (db_read_metadata(&db->super)) goto cleanup;

    dcp_std_prof_init(&db->prof, &db->abc);
    assert(db->super.prof_typeid == DCP_STD_PROFILE);
    return db;

cleanup:
    free(db);
    return NULL;
}

struct dcp_std_db *dcp_std_db_openw(FILE *restrict fd, struct dcp_std_cfg cfg)
{
    struct dcp_std_db *db = malloc(sizeof *db);
    if (!db)
    {
        error(DCP_OUTOFMEM, "failed to alloc db");
        return NULL;
    }
    db_init(&db->super, DCP_STD_PROFILE);
    db->abc = *cfg.abc;
    dcp_std_prof_init(&db->prof, &db->abc);

    if (db_openw(&db->super, fd)) goto cleanup;
    if (db_write_magic_number(&db->super)) goto cleanup;
    if (db_write_prof_type(&db->super)) goto cleanup;
    if (db_write_float_size(&db->super)) goto cleanup;
    if (write_abc(db->super.file.fd, &db->abc)) goto cleanup;

    return db;

cleanup:
    fclose(db->super.dp.fd);
    fclose(db->super.mt.file.fd);
    free(db);
    return NULL;
}

enum dcp_rc dcp_std_db_close(struct dcp_std_db *db)
{
    return db_close(&db->super);
}

struct imm_abc const *dcp_std_db_abc(struct dcp_std_db const *db)
{
    return &db->abc;
}

enum dcp_rc dcp_std_db_read(struct dcp_std_db *db, struct dcp_std_prof *prof)
{
    if (db_end(&db->super)) return error(DCP_RUNTIMEERROR, "end of profiles");
    prof->super.idx = db->super.profiles.curr_idx++;
    prof->super.mt = db_meta(&db->super, prof->super.idx);
    return std_prof_read(&prof->super, db->super.file.fd);
}

enum dcp_rc dcp_std_db_write(struct dcp_std_db *db,
                             struct dcp_std_prof const *prof)
{
    /* if ((rc = db_check_write_prof_ready(&db->super, &prof->super))) return
     * rc; */

    enum dcp_rc rc = DCP_SUCCESS;
    if ((rc = db_write_prof_meta(&db->super, &prof->super))) return rc;
    if ((rc = std_prof_write(&prof->super, db->super.dp.fd))) return rc;
    db->super.profiles.size++;
    return rc;
}

struct dcp_std_prof *dcp_std_db_profile(struct dcp_std_db *db)
{
    return &db->prof;
}

struct dcp_db *dcp_std_db_super(struct dcp_std_db *db) { return &db->super; }

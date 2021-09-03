#include "db.h"
#include "dcp/db.h"
#include "dcp/rc.h"
#include "dcp/std_cfg.h"
#include "dcp/std_profile.h"
#include "error.h"

struct dcp_std_db
{
    struct dcp_db super;
    struct dcp_std_cfg cfg;
    struct imm_abc abc;
    struct dcp_std_prof prof;
};

static inline struct dcp_std_db *new_std_db(void)
{
    struct dcp_std_db *db = calloc(1, sizeof(*db));
    db_init(&db->super);
    db->abc = imm_abc_empty;
    return db;
}

struct dcp_std_db *dcp_std_db_openr(FILE *restrict fd)
{
    struct dcp_std_db *db = new_std_db();
    db->super.file.fd = fd;
    xcmp_init(&db->super.file.ctx, db->super.file.fd);
    db->super.file.mode = DB_OPEN_READ;

    if (db_read_magic_number(&db->super)) goto cleanup;
    if (db_read_prof_type(&db->super)) goto cleanup;
    if (db_read_float_bytes(&db->super)) goto cleanup;

    if (db->cfg.prof_typeid == DCP_PROTEIN_PROFILE)
    {
        if (float_bytes == 4)
        {
            float e = 0;
            EREAD(!cmp_read_float(&db->file.ctx, &e));
            db->cfg.pro.epsilon = (imm_float)e;
        }
        else
        {
            double e = 0;
            EREAD(!cmp_read_double(&db->file.ctx, &e));
            db->cfg.pro.epsilon = (imm_float)e;
        }
        if (db->cfg.pro.epsilon < 0 || db->cfg.pro.epsilon > 1)
        {
            error(DCP_PARSEERROR, "wrong epsilon");
            goto cleanup;
        }

        uint8_t edist = 0;
        EREAD(!cmp_read_u8(&db->file.ctx, &edist));
        db->cfg.pro.edist = edist;

        if (imm_abc_read(&db->nuclt.super, db->file.fd))
        {
            error(DCP_IOERROR, "failed to read nuclt alphabet");
            goto cleanup;
        }

        if (imm_abc_read(&db->amino.super, db->file.fd))
        {
            error(DCP_IOERROR, "failed to read amino alphabet");
            goto cleanup;
        }
    }
    else
    {
        if (imm_abc_read(&db->abc, db->file.fd))
        {
            error(DCP_IOERROR, "failed to read alphabet");
            goto cleanup;
        }
    }

    EREAD(!cmp_read_u32(&db->file.ctx, &db->profiles.size));
    if (db->profiles.size > MAX_NPROFILES)
    {
        error(DCP_RUNTIMEERROR, "too many profiles");
        goto cleanup;
    }

    if (read_metadata(db)) goto cleanup;

    if (db->cfg.prof_typeid == DCP_STD_PROFILE)
        dcp_std_prof_init(&db->prof.std, &db->abc);

#if 0
    if (db->cfg.prof_typeid == DCP_PROTEIN_PROFILE)
        dcp_pro_profile_init(&db->prof.pro, &db->abc);
#endif

    return db;

cleanup:
    free(db);
    return NULL;
}

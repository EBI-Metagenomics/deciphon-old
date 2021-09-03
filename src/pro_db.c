#include "dcp/pro_db.h"
#include "db.h"
#include "dcp/pro_cfg.h"
#include "dcp/pro_profile.h"
#include "error.h"

struct dcp_pro_db
{
    struct dcp_db super;
    struct dcp_pro_cfg cfg;
    struct dcp_pro_prof prof;
};

static inline struct dcp_pro_db *new_pro_db(void)
{
    struct dcp_pro_db *db = calloc(1, sizeof(*db));
    db_init(&db->super);
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

struct dcp_pro_db *dcp_pro_db_openr(FILE *restrict fd)
{
    struct dcp_pro_db *db = new_pro_db();
    db_openr(&db->super, fd);

    if (db_read_magic_number(&db->super)) goto cleanup;
    if (db_read_prof_type(&db->super)) goto cleanup;
    if (db_read_float_bytes(&db->super)) goto cleanup;
    if (read_epsilon(&db->super.file.ctx, db->super.float_bytes,
                     &db->cfg.epsilon))
        goto cleanup;

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

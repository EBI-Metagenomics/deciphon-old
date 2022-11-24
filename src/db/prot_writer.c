#include "db/prot_writer.h"
#include "db/types.h"
#include "logy.h"
#include "model/model.h"
#include "rc.h"

static enum rc pack_entry_dist(struct lip_file *file,
                               enum entry_dist const *edist)
{
    if (!lip_write_cstr(file, "entry_dist")) return eio("write key");
    if (!lip_write_int(file, *edist)) return eio("write entry dist");
    return RC_OK;
}

static enum rc pack_epsilon(struct lip_file *file, imm_float const *epsilon)
{
    if (!lip_write_cstr(file, "epsilon")) return eio("write key");
    if (!lip_write_float(file, *epsilon)) return eio("write epsilon");
    return RC_OK;
}

static enum rc pack_nuclt(struct lip_file *file, struct imm_nuclt const *nuclt)
{
    if (!lip_write_cstr(file, "abc")) return eio("write key");
    if (imm_abc_pack(&nuclt->super, file)) return eio("write nuclt abc");
    return RC_OK;
}

static enum rc pack_amino(struct lip_file *file, struct imm_amino const *amino)
{
    if (!lip_write_cstr(file, "amino")) return eio("write amino key");
    if (imm_abc_pack(&amino->super, file)) return eio("write amino abc");
    return RC_OK;
}

static enum rc pack_entry_dist_cb(struct lip_file *file, void const *arg)
{
    return pack_entry_dist(file, arg);
}

static enum rc pack_epsilon_cb(struct lip_file *file, void const *arg)
{
    return pack_epsilon(file, arg);
}

static enum rc pack_nuclt_cb(struct lip_file *file, void const *arg)
{
    return pack_nuclt(file, arg);
}

static enum rc pack_amino_cb(struct lip_file *file, void const *arg)
{
    return pack_amino(file, arg);
}

enum rc prot_db_writer_open(struct prot_db_writer *db, FILE *fp,
                            struct imm_amino const *amino,
                            struct imm_nuclt const *nuclt, struct prot_cfg cfg)
{
    enum rc rc = db_writer_open(&db->super, fp);
    if (rc) return rc;
    db->amino = *amino;
    db->nuclt = *nuclt;
    imm_nuclt_code_init(&db->code, &db->nuclt);
    db->cfg = cfg;

    imm_float const *epsilon = &db->cfg.epsilon;
    enum entry_dist const *entry_dist = &db->cfg.entry_dist;

    rc = db_writer_pack_magic_number(&db->super);
    if (rc) goto cleanup;

    rc = db_writer_pack_profile_typeid(&db->super, PROFILE_PROTEIN);
    if (rc) goto cleanup;

    rc = db_writer_pack_float_size(&db->super);
    if (rc) goto cleanup;

    rc = db_writer_pack_header_item(&db->super, pack_entry_dist_cb, entry_dist);
    if (rc) goto cleanup;

    rc = db_writer_pack_header_item(&db->super, pack_epsilon_cb, epsilon);
    if (rc) goto cleanup;

    rc = db_writer_pack_header_item(&db->super, pack_nuclt_cb, &db->nuclt);
    if (rc) goto cleanup;

    rc = db_writer_pack_header_item(&db->super, pack_amino_cb, &db->amino);
    if (rc) goto cleanup;

    return rc;

cleanup:
    return db_writer_close(&db->super, false);
}

static enum rc pack_profile(struct lip_file *file, void const *prof)
{
    return prot_profile_pack(prof, file);
}

enum rc prot_db_writer_pack_profile(struct prot_db_writer *db,
                                    struct prot_profile const *profile)
{
    return db_writer_pack_profile(&db->super, pack_profile, profile);
}

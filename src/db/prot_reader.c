#include "db/prot_reader.h"
#include "db/reader.h"
#include "expect.h"
#include "imm/imm.h"
#include "logy.h"
#include "model/model.h"

static enum rc unpack_entry_dist(struct lip_file *file, enum entry_dist *edist)
{
    if (!expect_map_key(file, "entry_dist")) return eio("read key");
    if (!lip_read_int(file, edist)) return eio("read entry dist");

    return (*edist <= ENTRY_DIST_NULL || *edist > ENTRY_DIST_OCCUPANCY)
               ? einval("invalid entry dist")
               : RC_OK;
}

static enum rc unpack_epsilon(struct lip_file *file, imm_float *epsilon)
{
    if (!expect_map_key(file, "epsilon")) return eio("read key");
    if (!lip_read_float(file, epsilon)) return eio("read epsilon");

    return (*epsilon < 0 || *epsilon > 1) ? einval("invalid epsilon") : RC_OK;
}

static enum rc unpack_nuclt(struct lip_file *file, struct imm_nuclt *nuclt)
{
    if (!expect_map_key(file, "abc")) return eio("read key");
    if (imm_abc_unpack(&nuclt->super, file)) return eio("read nuclt");
    return RC_OK;
}

static enum rc unpack_amino(struct lip_file *file, struct imm_amino *amino)
{
    if (!expect_map_key(file, "amino")) return eio("read key");
    if (imm_abc_unpack(&amino->super, file)) return eio("read amino");
    return RC_OK;
}

enum rc prot_db_reader_open(struct prot_db_reader *db, FILE *fp)
{
    enum rc rc = db_reader_open(&db->super, fp);
    if (rc) return rc;

    if (!expect_map_size(&db->super.file, 2)) return eio("read map");

    if (!expect_map_key(&db->super.file, "header")) return eio("read key");

    if (!expect_map_size(&db->super.file, 8)) return eio("read map");

    rc = db_reader_unpack_magic_number(&db->super);
    if (rc) goto cleanup;

    rc = db_reader_unpack_profile_typeid(&db->super, PROFILE_PROTEIN);
    if (rc) goto cleanup;

    rc = db_reader_unpack_float_size(&db->super);
    if (rc) goto cleanup;

    rc = unpack_entry_dist(&db->super.file, &db->cfg.entry_dist);
    if (rc) goto cleanup;

    rc = unpack_epsilon(&db->super.file, &db->cfg.epsilon);
    if (rc) goto cleanup;

    rc = unpack_nuclt(&db->super.file, &db->nuclt);
    if (rc) goto cleanup;

    rc = unpack_amino(&db->super.file, &db->amino);
    if (rc) goto cleanup;

    imm_nuclt_code_init(&db->code, &db->nuclt);

    rc = db_reader_unpack_profile_sizes(&db->super);
    if (rc) goto cleanup;

    return rc;

cleanup:
    db_reader_close(&db->super);
    return rc;
}

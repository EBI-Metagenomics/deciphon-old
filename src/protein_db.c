#include "protein_db.h"
#include "db.h"
#include "entry_dist.h"
#include "logger.h"
#include "protein_cfg.h"
#include "protein_profile.h"
#include "rc.h"
#include "third-party/cmp.h"
#include "xcmp.h"
#include "xmath.h"

struct protein_db const protein_db_default = {0};

static enum rc read_epsilon(struct cmp_ctx_s *cmp, unsigned float_bytes,
                            imm_float *epsilon)
{
    if (float_bytes == 4)
    {
        float e = 0;
        if (!cmp_read_float(cmp, &e))
            return error(IOERROR, "failed to read epsilon");
        *epsilon = (imm_float)e;
    }
    else
    {
        double e = 0;
        if (!cmp_read_double(cmp, &e))
            return error(IOERROR, "failed to read epsilon");
        *epsilon = (imm_float)e;
    }

    if (*epsilon < 0 || *epsilon > 1)
        return error(PARSEERROR, "invalid epsilon");

    return DONE;
}

static enum rc write_epsilon(struct cmp_ctx_s *cmp, unsigned float_bytes,
                             imm_float epsilon)
{
    if (float_bytes == 4)
    {
        float e = (float)epsilon;
        if (!cmp_write_float(cmp, e))
            return error(IOERROR, "failed to write epsilon");
    }
    else
    {
        double e = (double)epsilon;
        if (!cmp_write_double(cmp, e))
            return error(IOERROR, "failed to write epsilon");
    }

    return DONE;
}

static enum rc read_entry_dist(struct cmp_ctx_s *cmp,
                               enum entry_dist *edist)
{
    uint8_t val = 0;
    if (!cmp_read_u8(cmp, &val))
        return error(IOERROR, "failed to read entry distribution");
    *edist = val;
    return DONE;
}

static enum rc write_entry_dist(struct cmp_ctx_s *cmp,
                                enum entry_dist edist)
{
    if (!cmp_write_u8(cmp, (uint8_t)edist))
        return error(IOERROR, "failed to write entry distribution");
    return DONE;
}

static enum rc read_nuclt(FILE *restrict fd, struct imm_nuclt *nuclt)
{
    if (imm_abc_read(&nuclt->super, fd))
        return error(IOERROR, "failed to read nuclt alphabet");
    return DONE;
}

static enum rc write_nuclt(FILE *restrict fd, struct imm_nuclt const *nuclt)
{
    if (imm_abc_write(&nuclt->super, fd))
        return error(IOERROR, "failed to write nuclt alphabet");
    return DONE;
}

static enum rc read_amino(FILE *restrict fd, struct imm_amino *amino)
{
    if (imm_abc_read(&amino->super, fd))
        return error(IOERROR, "failed to read amino alphabet");
    return DONE;
}

static enum rc write_amino(FILE *restrict fd, struct imm_amino const *amino)
{
    if (imm_abc_write(&amino->super, fd))
        return error(IOERROR, "failed to write amino alphabet");
    return DONE;
}

static void protein_db_init(struct protein_db *db)
{
    db_init(&db->super, PROTEIN_PROFILE);
    db->amino = imm_amino_empty;
    db->nuclt = imm_nuclt_empty;
    db->code = imm_nuclt_code_empty;
    db->code.nuclt = &db->nuclt;
    protein_profile_init(&db->prof, &db->amino, &db->code, PROTEIN_CFG_DEFAULT);
}

enum rc protein_db_setup_multi_readers(struct protein_db *db, unsigned nfiles,
                                       FILE *fp[])
{
    unsigned n = db_nprofiles(&db->super);
    assert(nfiles <= n);

    enum rc rc = DONE;
    struct protein_profile *prof = protein_db_profile(db);
    unsigned part = 0;
    rc = db_current_offset(&db->super, db->super.partition_offset + part);
    part++;
    unsigned size = 0;
    while (!db_end(&db->super) && part < nfiles)
    {
        if ((rc = protein_db_read(db, prof))) return rc;

        size++;
        if (size >= xmath_partition_size(n, nfiles, part - 1))
        {
            rc = db_current_offset(&db->super,
                                   db->super.partition_offset + part);
            ++part;
            size = 0;
        }
    }

    while (!db_end(&db->super))
    {
        if (!(rc = protein_db_read(db, prof))) return rc;
    }

    if (!db_end(&db->super)) return rc;
    rc = db_current_offset(&db->super, db->super.partition_offset + part);

    if (!db_end(&db->super)) return rc;
    db->super.npartitions = nfiles;
    db_set_files(&db->super, nfiles, fp);
    return DONE;
}

enum rc protein_db_openr(struct protein_db *db, FILE *restrict fd)
{
    protein_db_init(db);
    db_openr(&db->super, fd);

    struct cmp_ctx_s *cmp = &db->super.file.cmp[0];
    imm_float *epsilon = &db->prof.cfg.epsilon;

    enum rc rc = DONE;
    if ((rc = db_read_magic_number(&db->super))) return rc;
    if ((rc = db_read_prof_type(&db->super))) return rc;
    if ((rc = db_read_float_size(&db->super))) return rc;
    if ((rc = read_entry_dist(cmp, &db->prof.cfg.entry_dist))) return rc;
    if ((rc = read_epsilon(cmp, db->super.float_size, epsilon))) return rc;
    if ((rc = read_nuclt(fd, &db->nuclt))) return rc;
    if ((rc = read_amino(fd, &db->amino))) return rc;
    if ((rc = db_read_nprofiles(&db->super))) return rc;
    if ((rc = db_read_metadata(&db->super))) return rc;

    imm_code_init(&db->code.super, imm_super(&db->nuclt));
    assert(db->super.prof_typeid == PROTEIN_PROFILE);
    return db_record_first_partition_offset(&db->super);
}

enum rc protein_db_openw(struct protein_db *db, FILE *restrict fd,
                         struct imm_amino const *amino,
                         struct imm_nuclt const *nuclt, struct protein_cfg cfg)
{
    protein_db_init(db);
    db->amino = *amino;
    db->nuclt = *nuclt;
    imm_nuclt_code_init(&db->code, &db->nuclt);

    struct cmp_ctx_s *cmp = &db->super.file.cmp[0];
    unsigned float_size = db->super.float_size;
    imm_float epsilon = db->prof.cfg.epsilon;

    enum rc rc = DONE;
    if ((rc = db_openw(&db->super, fd))) goto cleanup;
    if ((rc = db_write_magic_number(&db->super))) goto cleanup;
    if ((rc = db_write_prof_type(&db->super))) goto cleanup;
    if ((rc = db_write_float_size(&db->super))) goto cleanup;
    if ((rc = write_entry_dist(cmp, db->prof.cfg.entry_dist))) goto cleanup;
    if ((rc = write_epsilon(cmp, float_size, epsilon))) goto cleanup;
    if ((rc = write_nuclt(fd, &db->nuclt))) goto cleanup;
    if ((rc = write_amino(fd, &db->amino))) goto cleanup;

    return rc;

cleanup:
    xcmp_close(&db->super.mt.file.cmp);
    xcmp_close(&db->super.dp.cmp);
    return rc;
}

enum rc protein_db_close(struct protein_db *db)
{
    enum rc rc = db_close(&db->super);
    protein_profile_del(&db->prof);
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
    return db->prof.cfg;
}

enum rc protein_db_read(struct protein_db *db, struct protein_profile *prof)
{
    if (db_end(&db->super)) return error(FAIL, "end of profiles");
    prof->super.idx = db->super.profiles.curr_idx++;
    prof->super.mt = db_meta(&db->super, prof->super.idx);
    return protein_profile_read(prof, &db->super.file.cmp[0]);
}

enum rc protein_db_write(struct protein_db *db,
                         struct protein_profile const *prof)
{
    /* TODO: db_check_write_prof_ready(&db->super, &prof->super) */
    enum rc rc = DONE;
    if ((rc = db_write_prof_meta(&db->super, &prof->super))) return rc;
    if ((rc = protein_profile_write(prof, &db->super.dp.cmp))) return rc;
    db->super.profiles.size++;
    return rc;
}

struct protein_profile *protein_db_profile(struct protein_db *db)
{
    return &db->prof;
}

struct db *protein_db_super(struct protein_db *db) { return &db->super; }

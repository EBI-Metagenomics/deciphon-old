#include "db/press.h"
#include "core/logging.h"
#include "core/pp.h"
#include "zc.h"

static enum rc count_profiles(struct db_press *p);
static enum rc prepare_writer(struct db_press *p, FILE *fp);
static enum rc finish_writer(struct db_press *p);
static void prepare_reader(struct db_press *p, FILE *fp);
static enum rc finish_reader(struct db_press *p);
static enum rc profile_write(struct db_press *p);

enum rc db_press_init(struct db_press *p, FILE *hmm_fp, FILE *db_fp)
{
    prepare_reader(p, hmm_fp);
    enum rc rc = count_profiles(p);
    if (rc) return rc;

    if ((rc = prepare_writer(p, db_fp))) return rc;

    protein_profile_init(&p->profile, p->reader.h3.prof.meta.acc,
                         &p->writer.db.amino, &p->writer.db.code,
                         p->writer.db.cfg);

    return rc;
}

#define HMMER3 "HMMER3/f"

unsigned db_press_nsteps(struct db_press const *p) { return p->profile_count; }

static enum rc count_profiles(struct db_press *p)
{
    unsigned count = 0;

    while (fgets(p->buffer, ARRAY_SIZE_OF(*p, buffer), p->reader.fp) != NULL)
    {
        if (!strncmp(p->buffer, HMMER3, strlen(HMMER3))) ++count;
    }

    if (!feof(p->reader.fp))
    {
        fclose(p->reader.fp);
        return eio("failed to count profiles");
    }

    return RC_OK;
}

enum rc db_press_step(struct db_press *p)
{
    enum rc rc = protein_h3reader_next(&p->reader.h3);
    return rc ? rc : profile_write(p);
}

enum rc db_press_cleanup(struct db_press *p)
{
    enum rc rc_r = finish_reader(p);
    enum rc rc_w = finish_writer(p);
    return rc_r ? rc_r : (rc_w ? rc_w : RC_OK);
}

static enum rc prepare_writer(struct db_press *p, FILE *fp)
{
    p->writer.fp = fp;

    struct imm_amino const *a = &imm_amino_iupac;
    struct imm_nuclt const *n = &imm_dna_iupac.super;
    struct protein_cfg cfg = PROTEIN_CFG_DEFAULT;

    enum rc rc = protein_db_writer_open(&p->writer.db, p->writer.fp, a, n, cfg);
    if (rc) fclose(p->writer.fp);

    return rc;
}

static enum rc finish_writer(struct db_press *p)
{
    if (!p->writer.fp) return RC_OK;
    enum rc rc = db_writer_close((struct db_writer *)&p->writer.db, true);
    if (rc)
    {
        fclose(p->writer.fp);
        return rc;
    }
    return fclose(p->writer.fp) ? eio("fclose") : RC_OK;
}

static void prepare_reader(struct db_press *p, FILE *fp)
{
    p->reader.fp = fp;

    struct imm_amino const *amino = &p->writer.db.amino;
    struct imm_nuclt_code const *code = &p->writer.db.code;
    struct protein_cfg cfg = p->writer.db.cfg;

    protein_h3reader_init(&p->reader.h3, amino, code, cfg, p->reader.fp);
}

static enum rc finish_reader(struct db_press *p)
{
    return fclose(p->reader.fp) ? eio("fclose") : RC_OK;
}

static enum rc profile_write(struct db_press *p)
{
    enum rc rc = protein_profile_absorb(&p->profile, &p->reader.h3.model);
    if (rc) return rc;

    zc_strlcpy(p->profile.super.accession, p->reader.h3.prof.meta.acc,
               PROFILE_ACC_SIZE);

    return protein_db_writer_pack_profile(&p->writer.db, &p->profile);
}

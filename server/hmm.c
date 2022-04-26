#include "hmm.h"
#include "c_toolbelt/c_toolbelt.h"
#include "deciphon/compiler.h"
#include "deciphon/db/protein_writer.h"
#include "deciphon/info.h"
#include "deciphon/logger.h"
#include "deciphon/model/protein_h3reader.h"
#include "deciphon/rc.h"
#include "deciphon/sched/api.h"
#include "file.h"
#include "hmr/hmr.h"
#include <stdint.h>
#include <string.h>

static struct sched_hmm hmm = {0};
static struct sched_db sched_db = {0};
static char db_filename[FILENAME_SIZE] = {0};
static struct api_rc api_rc = {0};

static struct protein_profile profile = {0};

static struct
{
    FILE *fp;
    struct protein_db_writer db;
} writer = {0};

static struct
{
    FILE *fp;
    struct protein_h3reader h3;
} reader = {0};

static enum rc profile_write(void)
{
    enum rc rc = protein_profile_absorb(&profile, &reader.h3.model);
    if (rc) return rc;

    CTB_STRLCPY(&profile.super, accession, reader.h3.prof.meta.acc);

    return protein_db_writer_pack_profile(&writer.db, &profile);
}

static void setup_db_filename(void)
{
    strcpy(db_filename, hmm.filename);
    db_filename[strlen(db_filename) - 3] = 'd';
    db_filename[strlen(db_filename) - 2] = 'c';
    db_filename[strlen(db_filename) - 1] = 'p';
}

static enum rc fetch_hmm(char const *filename, int64_t xxh3)
{
    FILE *fp = fopen(hmm.filename, "wb");
    enum rc rc = api_download_hmm(hmm.id, fp);
    if (rc)
    {
        fclose(fp);
        return rc;
    }
    return fclose(fp) ? eio("fclose") : RC_OK;
}

static enum rc prepare_writer(void)
{
    writer.fp = fopen(db_filename, "wb");
    if (!writer.fp) return eio("fopen");

    struct imm_amino const *amino = &imm_amino_iupac;
    struct imm_nuclt const *nuclt = &imm_dna_iupac.super;
    struct protein_cfg cfg = PROTEIN_CFG_DEFAULT;

    enum rc rc = RC_OK;

    rc = protein_db_writer_open(&writer.db, writer.fp, amino, nuclt, cfg);
    if (rc) fclose(writer.fp);

    return rc;
}

static enum rc finish_writer(void)
{
    if (!writer.fp) return RC_OK;
    enum rc rc = db_writer_close((struct db_writer *)&writer.db, true);
    if (rc)
    {
        fclose(writer.fp);
        return rc;
    }
    return fclose(writer.fp) ? eio("fclose") : RC_OK;
}

static enum rc prepare_reader(void)
{
    reader.fp = fopen(hmm.filename, "rb");
    if (!reader.fp) return eio("fopen");

    struct imm_amino const *amino = &writer.db.amino;
    struct imm_nuclt_code const *code = &writer.db.code;
    struct protein_cfg cfg = writer.db.cfg;

    protein_h3reader_init(&reader.h3, amino, code, cfg, reader.fp);
    return RC_OK;
}

static enum rc finish_reader(void)
{
    return fclose(reader.fp) ? eio("fclose") : RC_OK;
}

static void prepare_profile(void)
{
    protein_profile_init(&profile, reader.h3.prof.meta.acc, &writer.db.amino,
                         &writer.db.code, writer.db.cfg);
}

enum rc hmm_press(int64_t job_id, unsigned num_threads)
{
    (void)num_threads;
    enum rc rc = RC_OK;

    if ((rc = api_get_hmm_by_job_id(job_id, &hmm, &api_rc))) return rc;

    if ((rc = file_ensure_local(hmm.filename, hmm.xxh3, fetch_hmm))) return rc;
    int num_profs = hmr_count_profiles(hmm.filename);
    if (num_profs < 0)
    {
        efail("failed to count profiles");
        goto cleanup;
    }

    setup_db_filename();

    if ((rc = prepare_writer())) goto cleanup;
    if ((rc = prepare_reader())) goto cleanup;
    prepare_profile();

    int i = 0;
    int progress = 0;
    while (!(rc = protein_h3reader_next(&reader.h3)))
    {
        if ((rc = profile_write())) break;
        ++i;

        if ((i * 100) / num_profs > progress)
        {
            int inc = (i * 100) / num_profs - progress;
            progress += inc;
            info("Pressed %d%% of the profiles", progress);
            api_increment_job_progress(job_id, inc, &api_rc);
        }
    }
    progress = 100;
    info("Finishing up pressing");

    if (rc == RC_END) rc = RC_OK;

    enum rc rc_w = RC_OK;
    enum rc rc_r = RC_OK;

cleanup:
    rc_r = finish_reader();
    rc_w = finish_writer();

    rc = rc ? rc : (rc_r ? rc_r : (rc_w ? rc_w : RC_OK));
    if (rc) return rc;

    rc = api_upload_db(db_filename, &sched_db, &api_rc);
    if (rc) return rc;

    rc = api_set_job_state(job_id, SCHED_DONE, "", &api_rc);
    return rc;
}

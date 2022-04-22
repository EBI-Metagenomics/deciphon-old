#include "hmm.h"
#include "deciphon/compiler.h"
#include "deciphon/db/protein_writer.h"
#include "deciphon/info.h"
#include "deciphon/logger.h"
#include "deciphon/model/protein_h3reader.h"
#include "deciphon/rc.h"
#include "deciphon/sched/api.h"
#include "deciphon/strlcpy.h"
#include <stdint.h>
#include <string.h>

static struct sched_hmm hmm = {0};
static struct sched_db sched_db = {0};
static struct api_error error = {0};

static struct protein_db_writer db = {0};
static struct protein_profile profile = {0};
static struct protein_h3reader reader = {0};

static enum rc profile_write(void)
{
    enum rc rc = protein_profile_absorb(&profile, &reader.model);
    if (rc) return rc;

    dcp_strlcpy(profile.super.accession, reader.prof.meta.acc,
                ARRAY_SIZE_OF(profile.super, accession));

    return protein_db_writer_pack_profile(&db, &profile);
}

enum rc hmm_press(int64_t job_id, unsigned num_threads)
{
    (void)num_threads;
    enum rc rc = api_get_hmm_by_job_id(job_id, &hmm, &error);
    if (rc) return rc;

    FILE *hmm_fp = fopen(hmm.filename, "wb");
    rc = api_download_hmm(hmm.id, hmm_fp);
    if (rc) return rc;
    fclose(hmm_fp);

    char db_filename[FILENAME_SIZE] = {0};
    strcpy(db_filename, hmm.filename);
    db_filename[strlen(db_filename) - 3] = 'd';
    db_filename[strlen(db_filename) - 2] = 'c';
    db_filename[strlen(db_filename) - 1] = 'p';

    hmm_fp = fopen(hmm.filename, "rb");
    FILE *db_fp = fopen(db_filename, "wb");

    struct imm_nuclt const *nuclt = &imm_dna_iupac.super;
    rc = protein_db_writer_open(&db, db_fp, &imm_amino_iupac, nuclt,
                                PROTEIN_CFG_DEFAULT);
    if (rc) return rc;

    protein_h3reader_init(&reader, &db.amino, &db.code, db.cfg, hmm_fp);

    protein_profile_init(&profile, reader.prof.meta.acc, &db.amino, &db.code,
                         db.cfg);

    int i = 0;
    while (!(rc = protein_h3reader_next(&reader)))
    {
        if ((rc = profile_write()))
        {
            efail("failed to write profile");
            // goto cleanup;
        }
        ++i;
        if (i % 1000 == 0) info("Pressed %d profiles", i);
    }

    if (rc != RC_END)
    {
        error(rc, "failed to parse HMM file");
        // goto cleanup;
    }

    rc = db_writer_close((struct db_writer *)&db, true);

    fclose(db_fp);
    fclose(hmm_fp);

    rc = api_upload_db(db_filename, &sched_db, &error);
    if (rc) return rc;

    rc = api_set_job_state(job_id, SCHED_DONE, "", &error);
    return rc;
}

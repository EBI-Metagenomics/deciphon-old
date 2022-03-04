#include "deciphon/db/db.h"
#include "hope/hope.h"
#include "imm/imm.h"

void test_protein_db_writer(void);
void test_protein_db_reader(void);

int main(void)
{
    test_protein_db_writer();
    test_protein_db_reader();
    return hope_status();
}

void test_protein_db_writer(void)
{
    remove(TMPDIR "/db.dcp");

    struct imm_amino const *amino = &imm_amino_iupac;
    struct imm_nuclt const *nuclt = imm_super(&imm_dna_iupac);
    struct imm_nuclt_code code = {0};
    imm_nuclt_code_init(&code, nuclt);

    FILE *fp = fopen(TMPDIR "/db.dcp", "wb");
    NOTNULL(fp);

    struct protein_cfg cfg = protein_cfg(ENTRY_DIST_OCCUPANCY, 0.01f);
    struct protein_db_writer db = {0};
    EQ(protein_db_writer_open(&db, fp, amino, nuclt, cfg), RC_OK);

    struct protein_profile prof = {0};
    protein_profile_init(&prof, "accession0", amino, &code, cfg);

    unsigned core_size = 2;
    protein_profile_sample(&prof, 1, core_size);
    EQ(protein_db_writer_pack_profile(&db, &prof), RC_OK);

    protein_profile_sample(&prof, 2, core_size);
    EQ(protein_db_writer_pack_profile(&db, &prof), RC_OK);

    profile_del((struct profile *)&prof);
    EQ(db_writer_close((struct db_writer *)&db, true), RC_OK);
    fclose(fp);
}

void test_protein_db_reader(void)
{
    FILE *fp = fopen(TMPDIR "/db.dcp", "rb");
    NOTNULL(fp);
    struct protein_db_reader db = {0};
    EQ(protein_db_reader_open(&db, fp), RC_OK);

    EQ(db.super.nprofiles, 2);
    EQ(db.super.profile_typeid, PROFILE_PROTEIN);

    struct imm_abc const *abc = imm_super(&db.nuclt);
    EQ(imm_abc_typeid(abc), IMM_DNA);

    double logliks[] = {-2720.38142805010, -2854.53369140625};

    unsigned nprofs = 0;
    struct imm_prod prod = imm_prod();
    enum rc rc = RC_OK;
    struct profile_reader reader = {0};
    EQ(profile_reader_setup(&reader, (struct db_reader *)&db, 1), RC_OK);
    struct profile *prof = 0;
    while ((rc = profile_reader_next(&reader, 0, &prof)) != RC_END)
    {
        EQ(profile_typeid(prof), PROFILE_PROTEIN);
        struct imm_task *task = imm_task_new(profile_alt_dp(prof));
        struct imm_seq seq = imm_seq(imm_str(imm_example2_seq), abc);
        EQ(imm_task_setup(task, &seq), IMM_SUCCESS);
        EQ(imm_dp_viterbi(profile_alt_dp(prof), task, &prod), IMM_SUCCESS);
        CLOSE(prod.loglik, logliks[nprofs]);
        imm_del(task);
        ++nprofs;
    }
    EQ(nprofs, 2);

    imm_del(&prod);
    profile_reader_del(&reader);
    db_reader_close((struct db_reader *)&db);
    fclose(fp);
}

#include "protein_db.h"
#include "hope/hope.h"
#include "imm/imm.h"
#include "profile_reader.h"
#include "protein_db_examples.h"

void test_db_protein_openw(void);
void test_db_protein_openr(void);

int main(void)
{
    test_db_protein_openw();
    test_db_protein_openr();
    return hope_status();
}

void test_db_protein_openw(void)
{
    remove(TMPDIR "/db.dcp");
    protein_db_examples_new_ex1(TMPDIR "/db.dcp");
}

void test_db_protein_openr(void)
{
    FILE *fd = fopen(TMPDIR "/db.dcp", "rb");
    NOTNULL(fd);
    struct protein_db db = {0};
    EQ(protein_db_openr(&db, fd), RC_DONE);

    EQ(db_float_size(&db.super), IMM_FLOAT_BYTES);
    EQ(db_profile_typeid(&db.super), PROFILE_PROTEIN);
    struct imm_nuclt const *nuclt = protein_db_nuclt(&db);
    struct imm_abc const *abc = imm_super(nuclt);
    EQ(imm_abc_typeid(abc), IMM_DNA);

    EQ(db_nprofiles(&db.super), 2);

    struct metadata mt[2] = {db_metadata(&db.super, 0),
                             db_metadata(&db.super, 1)};
    EQ(mt[0].name, "NAME0");
    EQ(mt[0].acc, "ACC0");
    EQ(mt[1].name, "NAME1");
    EQ(mt[1].acc, "ACC1");

    double logliks[] = {-2720.38142805010, -2854.53369140625};

    unsigned nprofs = 0;
    struct imm_prod prod = imm_prod();
    enum rc rc = RC_DONE;
    struct profile_reader reader;
    EQ(profile_reader_setup(&reader, (struct db *)&db, 1), RC_DONE);
    while ((rc = profile_reader_next(&reader, 0)) != RC_END)
    {
        struct profile *prof = profile_reader_profile(&reader, 0);
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
    EQ(db_close((struct db *)&db), RC_DONE);
    fclose(fd);
}

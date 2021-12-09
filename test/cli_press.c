#include "cli.h"
#include "hope/hope.h"
#include "imm/imm.h"
#include "profile_reader.h"
#include "protein_db.h"

void test_cli_press_write(void);
void test_cli_press_read(void);

int main(void)
{
    test_cli_press_write();
    test_cli_press_read();
    return hope_status();
}

void test_cli_press_write(void)
{
    int argc = 4;
    char *argv[1024] = {"dcp-press", ASSETS "/Pfam-A.5.hmm", "-o",
                        TMPDIR "Pfam-A.5.dcp"};
    EQ(cli_press(argc, argv), RC_DONE);
}

void test_cli_press_read(void)
{
    FILE *fd = fopen(TMPDIR "Pfam-A.5.dcp", "rb");
    NOTNULL(fd);
    struct protein_db db = {0};
    EQ(protein_db_openr(&db, fd), RC_DONE);

    EQ(db_float_size(&db.super), IMM_FLOAT_BYTES);
    EQ(db_profile_typeid(&db.super), PROFILE_PROTEIN);
    struct imm_nuclt const *nuclt = protein_db_nuclt(&db);
    struct imm_abc const *abc = imm_super(nuclt);
    EQ(imm_abc_typeid(abc), IMM_DNA);

    EQ(db_nprofiles(&db.super), 5);

    struct metadata mt[5] = {
        db_metadata(&db.super, 0), db_metadata(&db.super, 1),
        db_metadata(&db.super, 2), db_metadata(&db.super, 3),
        db_metadata(&db.super, 4)};

    EQ(mt[0].name, "1-cysPrx_C");
    EQ(mt[0].acc, "PF10417.11");
    EQ(mt[1].name, "120_Rick_ant");
    EQ(mt[1].acc, "PF12574.10");
    EQ(mt[2].name, "12TM_1");
    EQ(mt[2].acc, "PF09847.11");
    EQ(mt[3].name, "14-3-3");
    EQ(mt[3].acc, "PF00244.22");
    EQ(mt[4].name, "17kDa_Anti_2");
    EQ(mt[4].acc, "PF16998.7");

    double logliks[5] = {-2809.46254227552, -2811.42242382613,
                         -2814.51230144228, -2810.78946074378,
                         -2808.72185153672};

    unsigned nprofs = 0;
    struct imm_prod prod = imm_prod();
    struct profile_reader reader;
    EQ(profile_reader_setup(&reader, (struct db *)&db, 1), RC_DONE);
    struct profile *prof = profile_reader_profile(&reader, 0);
    enum rc rc = RC_DONE;
    while ((rc = profile_reader_next(&reader, 0)) != RC_END)
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
    EQ(nprofs, 5);

    profile_reader_del(&reader);
    imm_del(&prod);
    EQ(db_close((struct db *)&db), RC_DONE);
    fclose(fd);
}

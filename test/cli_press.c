#include "cli_press.h"
#include "hope/hope.h"
#include "imm/imm.h"
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
    EQ(cli_press(argc, argv), DONE);
}

void test_cli_press_read(void)
{
    FILE *fd = fopen(TMPDIR "Pfam-A.5.dcp", "rb");
    NOTNULL(fd);
    struct protein_db db = protein_db_default;
    EQ(protein_db_openr(&db, fd), DONE);

    EQ(db_float_size(&db.super), IMM_FLOAT_BYTES);
    EQ(db_prof_typeid(&db.super), PROTEIN_PROFILE);
    struct imm_nuclt const *nuclt = protein_db_nuclt(&db);
    struct imm_abc const *abc = imm_super(nuclt);
    EQ(imm_abc_typeid(abc), IMM_DNA);

    EQ(db_nprofiles(&db.super), 5);

    struct meta mt[5] = {db_meta(&db.super, 0), db_meta(&db.super, 1),
                         db_meta(&db.super, 2), db_meta(&db.super, 3),
                         db_meta(&db.super, 4)};

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
    struct protein_profile *p = protein_db_profile(&db);
    while (!db_end(&db.super))
    {
        EQ(protein_db_read(&db, p), DONE);
        EQ(profile_typeid(&p->super), PROTEIN_PROFILE);
        struct imm_task *task = imm_task_new(&p->alt.dp);
        struct imm_seq seq = imm_seq(imm_str(imm_example2_seq), abc);
        EQ(imm_task_setup(task, &seq), IMM_SUCCESS);
        EQ(imm_dp_viterbi(&p->alt.dp, task, &prod), IMM_SUCCESS);
        CLOSE(prod.loglik, logliks[p->super.idx]);
        imm_del(task);
        ++nprofs;
    }
    EQ(nprofs, 5);

    imm_del(&prod);
    EQ(protein_db_close(&db), DONE);
    fclose(fd);
}

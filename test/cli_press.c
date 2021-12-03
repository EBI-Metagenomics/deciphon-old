#include "dcp/dcp.h"
#include "hope/hope.h"

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
    EQ(dcp_cli_press(argc, argv), DCP_DONE);
}

void test_cli_press_read(void)
{
    FILE *fd = fopen(TMPDIR "Pfam-A.5.dcp", "rb");
    NOTNULL(fd);
    struct dcp_pro_db db = dcp_pro_db_default;
    EQ(dcp_pro_db_openr(&db, fd), DCP_DONE);

    EQ(dcp_db_float_size(dcp_super(&db)), IMM_FLOAT_BYTES);
    EQ(dcp_db_prof_typeid(dcp_super(&db)), DCP_PRO_PROFILE);
    struct imm_nuclt const *nuclt = dcp_pro_db_nuclt(&db);
    struct imm_abc const *abc = imm_super(nuclt);
    EQ(imm_abc_typeid(abc), IMM_DNA);

    EQ(dcp_db_nprofiles(dcp_super(&db)), 5);

    struct meta mt[5] = {
        dcp_db_meta(dcp_super(&db), 0), dcp_db_meta(dcp_super(&db), 1),
        dcp_db_meta(dcp_super(&db), 2), dcp_db_meta(dcp_super(&db), 3),
        dcp_db_meta(dcp_super(&db), 4)};

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
    struct dcp_pro_prof *p = dcp_pro_db_profile(&db);
    while (!dcp_db_end(dcp_super(&db)))
    {
        EQ(dcp_pro_db_read(&db, p), DCP_DONE);
        EQ(dcp_prof_typeid(dcp_super(p)), DCP_PRO_PROFILE);
        struct imm_task *task = imm_task_new(&p->alt.dp);
        struct imm_seq seq = imm_seq(imm_str(imm_example2_seq), abc);
        EQ(imm_task_setup(task, &seq), IMM_SUCCESS);
        EQ(imm_dp_viterbi(&p->alt.dp, task, &prod), IMM_SUCCESS);
        CLOSE(prod.loglik, logliks[dcp_super(p)->idx]);
        imm_del(task);
        ++nprofs;
    }
    EQ(nprofs, 5);

    imm_del(&prod);
    EQ(dcp_pro_db_close(&db), DCP_DONE);
    fclose(fd);
}

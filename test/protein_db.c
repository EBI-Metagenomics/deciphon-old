#include "dcp/dcp.h"
#include "hope/hope.h"
#include "protein_db_examples.h"

void test_db_protein_openw(void);
void test_db_protein_openr(void);

int main(void)
{
    test_db_protein_openw();
    test_db_protein_openr();
    return hope_status();
}

void test_db_protein_openw(void) { protein_db_examples_new_ex1(TMPDIR "/db.dcp"); }

void test_db_protein_openr(void)
{
    FILE *fd = fopen(TMPDIR "/db.dcp", "rb");
    NOTNULL(fd);
    struct protein_db db = protein_db_default;
    EQ(protein_db_openr(&db, fd), DCP_DONE);

    EQ(dcp_db_float_size(dcp_super(&db)), IMM_FLOAT_BYTES);
    EQ(dcp_db_prof_typeid(dcp_super(&db)), PROTEIN_PROFILE);
    struct imm_nuclt const *nuclt = protein_db_nuclt(&db);
    struct imm_abc const *abc = imm_super(nuclt);
    EQ(imm_abc_typeid(abc), IMM_DNA);

    EQ(dcp_db_nprofiles(dcp_super(&db)), 2);

    struct meta mt[2] = {dcp_db_meta(dcp_super(&db), 0),
                             dcp_db_meta(dcp_super(&db), 1)};
    EQ(mt[0].name, "NAME0");
    EQ(mt[0].acc, "ACC0");
    EQ(mt[1].name, "NAME1");
    EQ(mt[1].acc, "ACC1");

    unsigned nprofs = 0;
    struct imm_prod prod = imm_prod();
    struct protein_prof *p = protein_db_profile(&db);
    while (!dcp_db_end(dcp_super(&db)))
    {
        EQ(protein_db_read(&db, p), DCP_DONE);
        EQ(dcp_prof_typeid(dcp_super(p)), PROTEIN_PROFILE);
        if (dcp_super(p)->idx == 0)
        {
            struct imm_task *task = imm_task_new(&p->alt.dp);
            struct imm_seq seq = imm_seq(imm_str(imm_example2_seq), abc);
            EQ(imm_task_setup(task, &seq), IMM_SUCCESS);
            EQ(imm_dp_viterbi(&p->alt.dp, task, &prod), IMM_SUCCESS);
            CLOSE(prod.loglik, -2720.38142805010);
            imm_del(task);
        }
        ++nprofs;
    }
    EQ(nprofs, 2);

    imm_del(&prod);
    EQ(protein_db_close(&db), DCP_DONE);
    fclose(fd);
}

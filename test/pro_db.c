#include "dcp/dcp.h"
#include "hope/hope.h"

void test_db_pro_openw(void);
void test_db_pro_openr(void);

int main(void)
{
    test_db_pro_openw();
    test_db_pro_openr();
    return hope_status();
}

void test_db_pro_openw(void)
{
    struct imm_amino const *amino = &imm_amino_iupac;
    struct imm_nuclt const *nuclt = imm_super(&imm_dna_iupac);

    FILE *fd = fopen(TMPDIR "/db.dcp", "wb");
    NOTNULL(fd);

    struct dcp_pro_cfg cfg = dcp_pro_cfg(DCP_ENTRY_DIST_UNIFORM, 0.1f);
    struct dcp_pro_db *db = dcp_pro_db_openw(fd, amino, nuclt, cfg);
    NOTNULL(db);

    struct dcp_pro_prof *prof = dcp_pro_db_profile(db);

    dcp_pro_prof_sample(prof, 1, 2);
    dcp_prof_nameit(dcp_super(prof), dcp_meta("NAME0", "ACC0"));
    EQ(dcp_pro_db_write(db, prof), DCP_SUCCESS);

    dcp_pro_prof_sample(prof, 2, 2);
    dcp_prof_nameit(dcp_super(prof), dcp_meta("NAME1", "ACC1"));
    EQ(dcp_pro_db_write(db, prof), DCP_SUCCESS);

    EQ(dcp_pro_db_close(db), DCP_SUCCESS);
    fclose(fd);
}

void test_db_pro_openr(void)
{
    FILE *fd = fopen(TMPDIR "/db.dcp", "rb");
    NOTNULL(fd);
    struct dcp_pro_db *db = dcp_pro_db_openr(fd);
    NOTNULL(db);
    EQ(dcp_db_float_size(dcp_super(db)), IMM_FLOAT_BYTES);
    EQ(dcp_db_prof_typeid(dcp_super(db)), DCP_PROTEIN_PROFILE);
    struct imm_nuclt const *nuclt = dcp_pro_db_nuclt(db);
    struct imm_abc const *abc = imm_super(nuclt);
    EQ(imm_abc_typeid(abc), IMM_DNA);

    EQ(dcp_db_nprofiles(dcp_super(db)), 2);

    struct dcp_meta mt[2] = {dcp_db_meta(dcp_super(db), 0),
                             dcp_db_meta(dcp_super(db), 1)};
    EQ(mt[0].name, "NAME0");
    EQ(mt[0].acc, "ACC0");
    EQ(mt[1].name, "NAME1");
    EQ(mt[1].acc, "ACC1");

    unsigned nprofs = 0;
    struct imm_result result = imm_result();
    struct dcp_pro_prof *p = dcp_pro_db_profile(db);
    while (!dcp_db_end(dcp_super(db)))
    {
        EQ(dcp_pro_db_read(db, p), DCP_SUCCESS);
        EQ(dcp_prof_typeid(dcp_super(p)), DCP_PROTEIN_PROFILE);
        if (dcp_super(p)->idx == 0)
        {
            struct imm_task *task = imm_task_new(&p->alt.dp);
            struct imm_seq seq = imm_seq(imm_str(imm_example2_seq), abc);
            EQ(imm_task_setup(task, &seq), IMM_SUCCESS);
            EQ(imm_dp_viterbi(&p->alt.dp, task, &result), IMM_SUCCESS);
            CLOSE(result.loglik, -3002.36462999679);
            imm_del(task);
        }
        ++nprofs;
    }
    EQ(nprofs, 2);

    imm_del(&result);
    EQ(dcp_pro_db_close(db), DCP_SUCCESS);
    fclose(fd);
}

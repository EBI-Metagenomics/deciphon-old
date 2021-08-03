#include "dcp/dcp.h"
#include "hope/hope.h"
#include "imm/imm.h"

void test_pro_model(void);

int main(void)
{
    test_pro_model();
    return hope_status();
}

void test_pro_model(void)
{
    unsigned core_size = 3;
    struct dcp_pro_cfg cfg = {&imm_amino_iupac, imm_super(imm_gc_dna()),
                              DCP_ENTRY_DIST_OCCUPANCY, 0.01f};
    imm_float null_lprobs[IMM_AMINO_SIZE];
    imm_float null_lodds[IMM_AMINO_SIZE];
    imm_float match_lprobs1[IMM_AMINO_SIZE];
    imm_float match_lprobs2[IMM_AMINO_SIZE];
    imm_float match_lprobs3[IMM_AMINO_SIZE];
    struct dcp_pro_model_trans t[4];

    struct imm_rnd rnd = imm_rnd(942);
    imm_lprob_sample(&rnd, IMM_AMINO_SIZE, null_lprobs);
    imm_lprob_sample(&rnd, IMM_AMINO_SIZE, null_lodds);
    imm_lprob_sample(&rnd, IMM_AMINO_SIZE, match_lprobs1);
    imm_lprob_sample(&rnd, IMM_AMINO_SIZE, match_lprobs2);
    imm_lprob_sample(&rnd, IMM_AMINO_SIZE, match_lprobs3);

    for (unsigned i = 0; i < 4; ++i)
    {
        imm_lprob_sample(&rnd, DCP_PRO_MODEL_TRANS_SIZE, t[i].data);
        imm_lprob_normalize(DCP_PRO_MODEL_TRANS_SIZE, t[i].data);
    }

    struct dcp_pro_model *model =
        dcp_pro_model_new(cfg, null_lprobs, null_lodds);

    EQ(dcp_pro_model_setup(model, core_size), IMM_SUCCESS);

    EQ(dcp_pro_model_add_node(model, match_lprobs1), IMM_SUCCESS);
    EQ(dcp_pro_model_add_node(model, match_lprobs2), IMM_SUCCESS);
    EQ(dcp_pro_model_add_node(model, match_lprobs3), IMM_SUCCESS);

    EQ(dcp_pro_model_add_trans(model, t[0]), IMM_SUCCESS);
    EQ(dcp_pro_model_add_trans(model, t[1]), IMM_SUCCESS);
    EQ(dcp_pro_model_add_trans(model, t[2]), IMM_SUCCESS);
    EQ(dcp_pro_model_add_trans(model, t[3]), IMM_SUCCESS);

    struct dcp_pro_profile p;
    dcp_pro_profile_init(&p, cfg);

    dcp_profile_nameit(dcp_super(&p), dcp_meta("NAME0", "ACC0"));
    EQ(dcp_pro_profile_absorb(&p, model), IMM_SUCCESS);

    dcp_del(&p);
    dcp_del(model);
}

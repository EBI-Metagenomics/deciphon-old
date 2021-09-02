#include "dcp/dcp.h"
#include "hmr/hmr.h"
#include "hope/hope.h"
#include "imm/imm.h"

int main(void)
{
    unsigned core_size = 3;
    struct dcp_pro_cfg cfg = {&imm_amino_iupac, imm_super(imm_gc_dna()),
                              DCP_ENTRY_DIST_OCCUPANCY, 0.01f};
    imm_float null_lprobs[IMM_AMINO_SIZE];
    imm_float null_lodds[IMM_AMINO_SIZE];
    imm_float match_lprobs1[IMM_AMINO_SIZE];
    imm_float match_lprobs2[IMM_AMINO_SIZE];
    imm_float match_lprobs3[IMM_AMINO_SIZE];
    struct dcp_pro_trans t[4];

    struct imm_rnd rnd = imm_rnd(942);
    imm_lprob_sample(&rnd, IMM_AMINO_SIZE, null_lprobs);
    imm_lprob_sample(&rnd, IMM_AMINO_SIZE, null_lodds);
    imm_lprob_sample(&rnd, IMM_AMINO_SIZE, match_lprobs1);
    imm_lprob_sample(&rnd, IMM_AMINO_SIZE, match_lprobs2);
    imm_lprob_sample(&rnd, IMM_AMINO_SIZE, match_lprobs3);

    for (unsigned i = 0; i < 4; ++i)
    {
        imm_lprob_sample(&rnd, DCP_PRO_TRANS_SIZE, t[i].data);
        imm_lprob_normalize(DCP_PRO_TRANS_SIZE, t[i].data);
    }

    struct dcp_pro_model model;
    dcp_pro_model_init(&model, cfg, null_lprobs);

    EQ(dcp_pro_model_setup(&model, core_size), DCP_SUCCESS);

    EQ(dcp_pro_model_add_node(&model, match_lprobs1), DCP_SUCCESS);
    EQ(dcp_pro_model_add_node(&model, match_lprobs2), DCP_SUCCESS);
    EQ(dcp_pro_model_add_node(&model, match_lprobs3), DCP_SUCCESS);

    EQ(dcp_pro_model_add_trans(&model, t[0]), DCP_SUCCESS);
    EQ(dcp_pro_model_add_trans(&model, t[1]), DCP_SUCCESS);
    EQ(dcp_pro_model_add_trans(&model, t[2]), DCP_SUCCESS);
    EQ(dcp_pro_model_add_trans(&model, t[3]), DCP_SUCCESS);

    struct dcp_pro_prof p;
    dcp_pro_prof_init(&p, cfg);

    dcp_prof_nameit(dcp_super(&p), dcp_meta("NAME0", "ACC0"));
    EQ(dcp_pro_prof_absorb(&p, &model), DCP_SUCCESS);

    dcp_del(&p);
    dcp_del(&model);
    return hope_status();
}

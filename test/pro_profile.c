#include "data.h"
#include "dcp/dcp.h"
#include "hope/hope.h"
#include "imm/imm.h"

void test_3core_nodes(void);

int main(void)
{
    test_3core_nodes();
    return hope_status();
}

void test_3core_nodes(void)
{
    struct pp_3core_nodes data = pp_3core_nodes_data();
    struct dcp_pro_model *m = dcp_pro_model_new(
        &data.amino, imm_super(data.dna), data.null_lprobs, data.null_lodds,
        data.epsilon, data.core_size, data.edistr);

    dcp_pro_model_add_node(m, data.match_lprobs1);
    dcp_pro_model_add_node(m, data.match_lprobs2);
    dcp_pro_model_add_node(m, data.match_lprobs3);

    dcp_pro_model_add_trans(m, data.trans0);
    dcp_pro_model_add_trans(m, data.trans1);
    dcp_pro_model_add_trans(m, data.trans2);
    dcp_pro_model_add_trans(m, data.trans3);

    struct dcp_pro_profile *p = dcp_pro_profile_new(
        &data.amino, imm_super(data.dna), data.edistr, data.epsilon);

    char str[] = "ATGAAACGCATTAGCACCACCATTACCACCAC"
                 "CATCACCATTACCACAGGTAACGGTGCGGGC";

    struct imm_seq seq = imm_seq(imm_str(str), imm_super(imm_super(data.dna)));

    unsigned len = (unsigned)strlen(str);
    bool multihits = true;
    bool hmmer3_compat = false;
    dcp_pro_profile_init(p, m);
    dcp_pro_profile_setup(p, len, multihits, hmmer3_compat);

    struct imm_dp const *ndp = dcp_pro_profile_null_dp(p);
    struct imm_task *ntask = imm_task_new(ndp);
    struct imm_result result = imm_result();
    imm_task_setup(ntask, &seq);
    imm_dp_viterbi(ndp, ntask, &result);

    CLOSE(result.loglik, -87.6962203979);

    EQ(imm_path_nsteps(&result.path), 21);

    for (unsigned i = 0; i < 21; ++i)
    {
        EQ(imm_path_step(&result.path, i)->seqlen, 3);
        EQ(imm_path_step(&result.path, i)->state_id, DCP_PRO_MODEL_R_ID);
    }

    imm_result_reset(&result);

    struct imm_dp const *adp = dcp_pro_profile_alt_dp(p);
    struct imm_task *atask = imm_task_new(adp);
    result = imm_result();
    imm_task_setup(atask, &seq);
    imm_dp_viterbi(adp, atask, &result);

    CLOSE(result.loglik, -91.9514160156);

    EQ(imm_path_nsteps(&result.path), 25);

    EQ(imm_path_step(&result.path, 0)->seqlen, 0);
    EQ(imm_path_step(&result.path, 0)->state_id, DCP_PRO_MODEL_S_ID);

    EQ(imm_path_step(&result.path, 1)->seqlen, 0);
    EQ(imm_path_step(&result.path, 1)->state_id, DCP_PRO_MODEL_B_ID);

    EQ(imm_path_step(&result.path, 2)->seqlen, 3);
    EQ(imm_path_step(&result.path, 2)->state_id, DCP_PRO_MODEL_MATCH_ID | 1U);

    EQ(imm_path_step(&result.path, 3)->seqlen, 3);
    EQ(imm_path_step(&result.path, 3)->state_id, DCP_PRO_MODEL_MATCH_ID | 2U);

    EQ(imm_path_step(&result.path, 4)->seqlen, 3);
    EQ(imm_path_step(&result.path, 4)->state_id, DCP_PRO_MODEL_MATCH_ID | 3U);

    EQ(imm_path_step(&result.path, 5)->seqlen, 0);
    EQ(imm_path_step(&result.path, 5)->state_id, DCP_PRO_MODEL_E_ID);

    for (unsigned i = 6; i < 24; ++i)
    {
        EQ(imm_path_step(&result.path, i)->seqlen, 3);
        EQ(imm_path_step(&result.path, i)->state_id, DCP_PRO_MODEL_C_ID);
    }

    EQ(imm_path_step(&result.path, 24)->seqlen, 0);
    EQ(imm_path_step(&result.path, 24)->state_id, DCP_PRO_MODEL_T_ID);

    imm_result_del(&result);
}

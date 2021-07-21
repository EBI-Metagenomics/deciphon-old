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
    struct dcp_pp *pp = dcp_pp_new(data.null_lprobs, data.null_lodds, data.cfg);

    dcp_pp_add_node(pp, data.match_lprobs1);
    dcp_pp_add_node(pp, data.match_lprobs2);
    dcp_pp_add_node(pp, data.match_lprobs3);

    dcp_pp_add_trans(pp, data.trans0);
    dcp_pp_add_trans(pp, data.trans1);
    dcp_pp_add_trans(pp, data.trans2);
    dcp_pp_add_trans(pp, data.trans3);

    char str[] = "ATGAAACGCATTAGCACCACCATTACCACCAC"
                 "CATCACCATTACCACAGGTAACGGTGCGGGC";

    struct imm_seq seq =
        imm_seq(imm_str(str), imm_super(imm_super(imm_gc_dna())));

    unsigned len = (unsigned)strlen(str);
    bool multihits = true;
    bool hmmer3_compat = false;
    /* dcp_pp_set_target_length(pp, len, multihits, hmmer3_compat); */
    dcp_pp_set_target_length2(pp, len, multihits, hmmer3_compat);

    struct imm_dp *ndp = dcp_pp_null_dp(pp);
    struct imm_task *ntask = imm_task_new(ndp);
    struct imm_result result = imm_result();
    imm_task_setup(ntask, &seq);
    imm_dp_viterbi(ndp, ntask, &result);

    CLOSE(result.loglik, -87.6962203979);

    EQ(imm_path_nsteps(&result.path), 21);

    for (unsigned i = 0; i < 21; ++i)
    {
        EQ(imm_path_step(&result.path, i)->seqlen, 3);
        EQ(imm_path_step(&result.path, i)->state_id, DCP_PP_R_ID);
    }

    imm_result_reset(&result);

    struct imm_dp *adp = dcp_pp_alt_dp(pp);
    struct imm_task *atask = imm_task_new(adp);
    result = imm_result();
    imm_task_setup(atask, &seq);
    imm_dp_viterbi(adp, atask, &result);

    CLOSE(result.loglik, -91.9514160156);

    EQ(imm_path_nsteps(&result.path), 25);

    EQ(imm_path_step(&result.path, 0)->seqlen, 0);
    EQ(imm_path_step(&result.path, 0)->state_id, DCP_PP_S_ID);

    EQ(imm_path_step(&result.path, 1)->seqlen, 0);
    EQ(imm_path_step(&result.path, 1)->state_id, DCP_PP_B_ID);

    EQ(imm_path_step(&result.path, 2)->seqlen, 3);
    EQ(imm_path_step(&result.path, 2)->state_id, DCP_PP_MATCH_ID | 1U);

    EQ(imm_path_step(&result.path, 3)->seqlen, 3);
    EQ(imm_path_step(&result.path, 3)->state_id, DCP_PP_MATCH_ID | 2U);

    EQ(imm_path_step(&result.path, 4)->seqlen, 3);
    EQ(imm_path_step(&result.path, 4)->state_id, DCP_PP_MATCH_ID | 3U);

    EQ(imm_path_step(&result.path, 5)->seqlen, 0);
    EQ(imm_path_step(&result.path, 5)->state_id, DCP_PP_E_ID);

    for (unsigned i = 6; i < 24; ++i)
    {
        EQ(imm_path_step(&result.path, i)->seqlen, 3);
        EQ(imm_path_step(&result.path, i)->state_id, DCP_PP_C_ID);
    }

    EQ(imm_path_step(&result.path, 24)->seqlen, 0);
    EQ(imm_path_step(&result.path, 24)->state_id, DCP_PP_T_ID);

    imm_result_del(&result);
}

#include "data.h"
#include "dcp/dcp.h"
#include "hope/hope.h"
#include "imm/imm.h"

void test_3core_nodes(void);
void test_pro_profile1(void);

int main(void)
{
    test_3core_nodes();
    test_pro_profile1();
    return hope_status();
}

void test_3core_nodes(void)
{
    struct dcp_pro_profile p;
    pro_profile_with_3cores(&p);
    struct dcp_profile *prof = dcp_super(&p);

    char str[] = "ATGAAACGCATTAGCACCACCATTACCACCAC"
                 "CATCACCATTACCACAGGTAACGGTGCGGGC";

    struct imm_seq seq = imm_seq(imm_str(str), prof->abc);

    unsigned len = (unsigned)strlen(str);
    bool multihits = true;
    bool hmmer3_compat = false;
    dcp_pro_profile_setup(&p, len, multihits, hmmer3_compat);

    struct imm_result result = imm_result();
    struct imm_dp const *ndp = &p.null.dp;
    struct imm_task *ntask = imm_task_new(ndp);
    imm_task_setup(ntask, &seq);
    imm_dp_viterbi(ndp, ntask, &result);

    CLOSE(result.loglik, -87.6962228106);

    EQ(imm_path_nsteps(&result.path), 21);

    for (unsigned i = 0; i < 21; ++i)
    {
        EQ(imm_path_step(&result.path, i)->seqlen, 3);
        EQ(imm_path_step(&result.path, i)->state_id, DCP_PRO_MODEL_R_ID);
    }

    imm_result_reset(&result);

    struct imm_dp const *adp = &p.alt.dp;
    struct imm_task *atask = imm_task_new(adp);
    imm_task_setup(atask, &seq);
    imm_dp_viterbi(adp, atask, &result);

    CLOSE(result.loglik, -93.3531942236);

    EQ(imm_path_nsteps(&result.path), 25);
    char name[8];

    EQ(imm_path_step(&result.path, 0)->seqlen, 0);
    EQ(imm_path_step(&result.path, 0)->state_id, DCP_PRO_MODEL_S_ID);
    dcp_pro_profile_state_name(imm_path_step(&result.path, 0)->state_id, name);
    EQ(name, "S");

    EQ(imm_path_step(&result.path, 1)->seqlen, 0);
    EQ(imm_path_step(&result.path, 1)->state_id, DCP_PRO_MODEL_B_ID);
    dcp_pro_profile_state_name(imm_path_step(&result.path, 1)->state_id, name);
    EQ(name, "B");

    EQ(imm_path_step(&result.path, 2)->seqlen, 3);
    EQ(imm_path_step(&result.path, 2)->state_id, DCP_PRO_MODEL_MATCH_ID | 1U);
    dcp_pro_profile_state_name(imm_path_step(&result.path, 2)->state_id, name);
    EQ(name, "M1");

    EQ(imm_path_step(&result.path, 3)->seqlen, 0);
    EQ(imm_path_step(&result.path, 3)->state_id, DCP_PRO_MODEL_E_ID);
    dcp_pro_profile_state_name(imm_path_step(&result.path, 3)->state_id, name);
    EQ(name, "E");

    EQ(imm_path_step(&result.path, 4)->seqlen, 3);
    EQ(imm_path_step(&result.path, 4)->state_id, DCP_PRO_MODEL_C_ID);
    dcp_pro_profile_state_name(imm_path_step(&result.path, 4)->state_id, name);
    EQ(name, "C");

    EQ(imm_path_step(&result.path, 5)->seqlen, 3);
    EQ(imm_path_step(&result.path, 5)->state_id, DCP_PRO_MODEL_C_ID);
    dcp_pro_profile_state_name(imm_path_step(&result.path, 5)->state_id, name);
    EQ(name, "C");

    for (unsigned i = 6; i < 24; ++i)
    {
        EQ(imm_path_step(&result.path, i)->seqlen, 3);
        EQ(imm_path_step(&result.path, i)->state_id, DCP_PRO_MODEL_C_ID);
    }

    EQ(imm_path_step(&result.path, 24)->seqlen, 0);
    EQ(imm_path_step(&result.path, 24)->state_id, DCP_PRO_MODEL_T_ID);

    imm_del(&result);
    imm_del(ntask);
    imm_del(atask);
    dcp_del(&p);
}

void test_pro_profile1(void)
{
    struct dcp_pro_profile p;
    dcp_pro_profile_sample(&p, 1, 2, DCP_ENTRY_DIST_UNIFORM, 0.1f);

    char const str[] = "ATGAAACGCATTAGCACCACCATTACCACCAC";
    struct imm_seq seq = imm_seq(imm_str(str), dcp_super(&p)->abc);

    unsigned len = (unsigned)strlen(str);
    bool multihits = true;
    bool hmmer3_compat = false;
    dcp_pro_profile_setup(&p, len, multihits, hmmer3_compat);

    struct imm_result result = imm_result();
    struct imm_dp const *ndp = &p.null.dp;
    struct imm_task *ntask = imm_task_new(ndp);
    imm_task_setup(ntask, &seq);
    imm_dp_viterbi(ndp, ntask, &result);

    CLOSE(result.loglik, -48.9272687711);

    EQ(imm_path_nsteps(&result.path), 11);
    char name[8];

    EQ(imm_path_step(&result.path, 0)->seqlen, 3);
    EQ(imm_path_step(&result.path, 0)->state_id, DCP_PRO_MODEL_R_ID);
    dcp_pro_profile_state_name(imm_path_step(&result.path, 0)->state_id, name);
    EQ(name, "R");

    EQ(imm_path_step(&result.path, 10)->seqlen, 2);
    EQ(imm_path_step(&result.path, 10)->state_id, DCP_PRO_MODEL_R_ID);
    dcp_pro_profile_state_name(imm_path_step(&result.path, 10)->state_id, name);
    EQ(name, "R");

    dcp_del(&p);
}

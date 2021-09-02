#include "dcp/dcp.h"
#include "hope/hope.h"
#include "imm/imm.h"

void test_pro_profile_uniform(void);
void test_pro_profile_occupancy(void);

int main(void)
{
    test_pro_profile_uniform();
    test_pro_profile_occupancy();
    return hope_status();
}

void test_pro_profile_uniform(void)
{
    struct dcp_pro_prof p;
    dcp_pro_prof_sample(&p, 1, 2, DCP_ENTRY_DIST_UNIFORM, 0.1f);

    char const str[] = "ATGAAACGCATTAGCACCACCATTACCACCAC";
    struct imm_seq seq = imm_seq(imm_str(str), dcp_super(&p)->abc);

    unsigned len = (unsigned)strlen(str);
    bool multi_hits = true;
    bool hmmer3_compat = false;
    dcp_pro_prof_setup(&p, len, multi_hits, hmmer3_compat);

    struct imm_result result = imm_result();
    struct imm_dp *dp = &p.null.dp;
    struct imm_task *task = imm_task_new(dp);
    imm_task_setup(task, &seq);
    imm_dp_viterbi(dp, task, &result);

    CLOSE(result.loglik, -48.9272687711);

    EQ(imm_path_nsteps(&result.path), 11);
    char name[8];

    EQ(imm_path_step(&result.path, 0)->seqlen, 3);
    EQ(imm_path_step(&result.path, 0)->state_id, DCP_PRO_ID_R);
    dcp_pro_prof_state_name(imm_path_step(&result.path, 0)->state_id, name);
    EQ(name, "R");

    EQ(imm_path_step(&result.path, 10)->seqlen, 2);
    EQ(imm_path_step(&result.path, 10)->state_id, DCP_PRO_ID_R);
    dcp_pro_prof_state_name(imm_path_step(&result.path, 10)->state_id, name);
    EQ(name, "R");

    imm_result_reset(&result);
    imm_del(task);

    dp = &p.alt.dp;
    task = imm_task_new(dp);
    imm_task_setup(task, &seq);
    imm_dp_viterbi(dp, task, &result);

    CLOSE(result.loglik, -55.5597178224);

    EQ(imm_path_nsteps(&result.path), 14);

    EQ(imm_path_step(&result.path, 0)->seqlen, 0);
    EQ(imm_path_step(&result.path, 0)->state_id, DCP_PRO_ID_S);
    dcp_pro_prof_state_name(imm_path_step(&result.path, 0)->state_id, name);
    EQ(name, "S");

    EQ(imm_path_step(&result.path, 13)->seqlen, 0);
    EQ(imm_path_step(&result.path, 13)->state_id, DCP_PRO_ID_T);
    dcp_pro_prof_state_name(imm_path_step(&result.path, 13)->state_id, name);
    EQ(name, "T");

    dcp_del(&p);
    imm_del(&result);
    imm_del(task);
}

void test_pro_profile_occupancy(void)
{
    struct dcp_pro_prof p;
    dcp_pro_prof_sample(&p, 1, 2, DCP_ENTRY_DIST_OCCUPANCY, 0.1f);

    char const str[] = "ATGAAACGCATTAGCACCACCATTACCACCAC";
    struct imm_seq seq = imm_seq(imm_str(str), dcp_super(&p)->abc);

    unsigned len = (unsigned)strlen(str);
    bool multi_hits = true;
    bool hmmer3_compat = false;
    dcp_pro_prof_setup(&p, len, multi_hits, hmmer3_compat);

    struct imm_result result = imm_result();
    struct imm_dp *dp = &p.null.dp;
    struct imm_task *task = imm_task_new(dp);
    imm_task_setup(task, &seq);
    imm_dp_viterbi(dp, task, &result);

    CLOSE(result.loglik, -48.9272687711);

    EQ(imm_path_nsteps(&result.path), 11);
    char name[8];

    EQ(imm_path_step(&result.path, 0)->seqlen, 3);
    EQ(imm_path_step(&result.path, 0)->state_id, DCP_PRO_ID_R);
    dcp_pro_prof_state_name(imm_path_step(&result.path, 0)->state_id, name);
    EQ(name, "R");

    EQ(imm_path_step(&result.path, 10)->seqlen, 2);
    EQ(imm_path_step(&result.path, 10)->state_id, DCP_PRO_ID_R);
    dcp_pro_prof_state_name(imm_path_step(&result.path, 10)->state_id, name);
    EQ(name, "R");

    imm_result_reset(&result);
    imm_del(task);

    dp = &p.alt.dp;
    task = imm_task_new(dp);
    imm_task_setup(task, &seq);
    imm_dp_viterbi(dp, task, &result);

    CLOSE(result.loglik, -54.3628743048);

    EQ(imm_path_nsteps(&result.path), 14);

    EQ(imm_path_step(&result.path, 0)->seqlen, 0);
    EQ(imm_path_step(&result.path, 0)->state_id, DCP_PRO_ID_S);
    dcp_pro_prof_state_name(imm_path_step(&result.path, 0)->state_id, name);
    EQ(name, "S");

    EQ(imm_path_step(&result.path, 13)->seqlen, 0);
    EQ(imm_path_step(&result.path, 13)->state_id, DCP_PRO_ID_T);
    dcp_pro_prof_state_name(imm_path_step(&result.path, 13)->state_id, name);
    EQ(name, "T");

    dcp_del(&p);
    imm_del(&result);
    imm_del(task);
}

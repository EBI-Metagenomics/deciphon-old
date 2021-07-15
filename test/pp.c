#include "dcp/dcp.h"
#include "hope/hope.h"
#include "imm/imm.h"

#define F(x) ((imm_float)(x))

void test_1(void);

int main(void)
{
    test_1();
    return hope_status();
}

void test_1(void)
{
    imm_float null_lprobs[IMM_AMINO_SIZE] =
        IMM_ARR(-2.540912081508539, -4.1890948987679115, -2.9276587578784476,
                -2.7056061901315998, -3.2262479321978232, -2.6663263733558105,
                -3.7757541131771606, -2.8300619150291904, -2.8227508671445096,
                -2.3395312748559522, -3.739255274772411, -3.1835424653853783,
                -3.0305224958425274, -3.2298381926580353, -2.916961759390943,
                -2.6833127114700477, -2.9174998187846013, -2.6979756205426138,
                -4.472958413679587, -3.4928752662451816);

    imm_float null_lodds[IMM_AMINO_SIZE] =
        IMM_ARR(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    imm_float epsilon = F(0.01);

    unsigned core_size = 3;
    enum dcp_entry_distr entry_distr = OCCUPANCY;
    struct dcp_pp *pp =
        dcp_pp_create(null_lprobs, null_lodds, epsilon, core_size, entry_distr);

    imm_float match_lprobs0[] = IMM_ARR(
        -4.25112, -5.66746, -5.45821, -5.2525, -4.2567, -4.83375, -5.7558,
        -3.58523, -4.98633, -2.78386, -0.25191, -5.3767, -5.38471, -5.32431,
        -5.04119, -4.6454, -4.62574, -3.6984, -6.12261, -4.97173);

    dcp_pp_add_node(pp, match_lprobs0);

    imm_float match_lprobs1[] = IMM_ARR(
        -3.57128, -5.79713, -3.93569, -3.24178, -5.33367, -4.17277, -4.07137,
        -4.66233, -1.31669, -4.02389, -4.86889, -1.6704, -4.52334, -3.18856,
        -1.37659, -3.53154, -3.72192, -4.30796, -6.0116, -4.8479);
    dcp_pp_add_node(pp, match_lprobs1);

    imm_float match_lprobs2[] = IMM_ARR(
        -2.7337, -5.25734, -2.75559, -2.32811, -4.59103, -1.94455, -3.63027,
        -4.07087, -2.20106, -3.5575, -4.30274, -2.64987, -3.04144, -2.68671,
        -2.89772, -2.68931, -2.71242, -3.6422, -5.6956, -4.28822);
    dcp_pp_add_node(pp, match_lprobs2);

    dcp_pp_add_trans(pp, (struct dcp_trans){.MM = F(-0.01083),
                                            .MI = F(-4.92694),
                                            .MD = F(-5.64929),
                                            .IM = F(-0.61958),
                                            .II = F(-0.77255),
                                            .DM = F(-0.0),
                                            .DD = IMM_LPROB_ZERO});
    dcp_pp_add_trans(pp, (struct dcp_trans){.MM = F(-0.01083),
                                            .MI = F(-4.92694),
                                            .MD = F(-5.64929),
                                            .IM = F(-0.61958),
                                            .II = F(-0.77255),
                                            .DM = F(-0.48576),
                                            .DD = F(-0.9551)});
    dcp_pp_add_trans(pp, (struct dcp_trans){.MM = F(-0.01083),
                                            .MI = F(-4.92694),
                                            .MD = F(-5.64929),
                                            .IM = F(-0.61958),
                                            .II = F(-0.77255),
                                            .DM = F(-0.48576),
                                            .DD = F(-0.9551)});
    dcp_pp_add_trans(pp, (struct dcp_trans){.MM = F(-0.00968),
                                            .MI = F(-4.64203),
                                            .MD = IMM_LPROB_ZERO,
                                            .IM = F(-0.61958),
                                            .II = F(-0.77255),
                                            .DM = F(-0.0),
                                            .DD = IMM_LPROB_ZERO});

    char str[] =
        "ATGAAACGCATTAGCACCACCATTACCACCACCATCACCATTACCACAGGTAACGGTGCGGGC";

    struct imm_seq seq =
        imm_seq(imm_str(str), imm_super(imm_super(imm_gc_dna())));

    unsigned len = (unsigned)strlen(str);
    bool multihits = true;
    bool hmmer3_compat = false;
    dcp_pp_set_target_length(pp, len, multihits, hmmer3_compat);

#if 0
    struct imm_dp *ndp = dcp_pp_null_new_dp(pp);
    struct imm_task *ntask = imm_task_new(ndp);
    struct imm_result result = imm_result();
    imm_task_setup(ntask, &seq);
    imm_dp_viterbi(ndp, ntask, &result);
    printf("NULL: %f\n", result.loglik);
#else

    /* struct imm_hmm *ahmm = dcp_pp_alt_hmm(pp); */
    /* imm_hmm_write_dot(ahmm, stdout, dcp_pp_state_name); */
    struct imm_dp *adp = dcp_pp_alt_new_dp(pp);
    struct imm_task *atask = imm_task_new(adp);
    struct imm_result result = imm_result();
    imm_task_setup(atask, &seq);
    imm_dp_viterbi(adp, atask, &result);
    printf("ALT: %f\n", result.loglik);
#endif
}

#include "cass/cass.h"
#include "deciphon/deciphon.h"
#include "helper.h"
#include "nmm/nmm.h"

#ifndef TMPDIR
#define TMPDIR ""
#endif

void test_output(void);
void test_input(void);
void test_small(void);
void test_pfam(void);

int main(void)
{
    test_output();
    test_input();
    test_small();
    /* test_pfam(); */
    return cass_status();
}

void test_output(void)
{
    struct nmm_base_abc const*   base = nmm_base_abc_create("ACGT", 'X');
    struct imm_abc const*        abc = nmm_base_abc_super(base);
    struct nmm_base_lprob const* basep =
        nmm_base_lprob_create(base, imm_log(0.25), imm_log(0.25), imm_log(0.5), zero());

    struct nmm_codon_lprob* codonp = nmm_codon_lprob_create(base);
    struct nmm_codon*       codon = nmm_codon_create(base);
    cass_cond(nmm_codon_set_triplet(codon, NMM_TRIPLET('A', 'T', 'G')) == 0);
    nmm_codon_lprob_set(codonp, codon, imm_log(0.8));
    cass_cond(nmm_codon_set_triplet(codon, NMM_TRIPLET('A', 'T', 'T')) == 0);
    nmm_codon_lprob_set(codonp, codon, imm_log(0.1));
    cass_cond(nmm_codon_set_triplet(codon, NMM_TRIPLET('C', 'C', 'C')) == 0);
    nmm_codon_lprob_set(codonp, codon, imm_log(0.1));
    struct nmm_codon_marg const* codont = nmm_codon_marg_create(codonp);

    struct imm_hmm* hmm = imm_hmm_create(abc);

    struct nmm_frame_state const* state1 = nmm_frame_state_create("S0", basep, codont, (imm_float)0.1);
    struct nmm_codon_state const* state2 = nmm_codon_state_create("S1", codonp);

    imm_hmm_add_state(hmm, nmm_frame_state_super(state1), imm_log(1.0));
    imm_hmm_add_state(hmm, nmm_codon_state_super(state2), imm_log(0.0001));
    imm_hmm_set_trans(hmm, nmm_frame_state_super(state1), nmm_codon_state_super(state2), imm_log(0.2));

    struct imm_path* path = imm_path_create();
    imm_path_append(path, imm_step_create(nmm_frame_state_super(state1), 1));
    struct imm_seq const* seq = imm_seq_create("A", abc);
    cass_close(imm_hmm_loglikelihood(hmm, seq, path), -6.0198640216);
    imm_seq_destroy(seq);
    imm_path_destroy(path);

    path = imm_path_create();
    imm_path_append(path, imm_step_create(nmm_frame_state_super(state1), 1));
    seq = imm_seq_create("C", abc);
    cass_close(imm_hmm_loglikelihood(hmm, seq, path), -7.118476310297789);
    imm_seq_destroy(seq);
    imm_path_destroy(path);

    struct imm_dp const* dp = imm_hmm_create_dp(hmm, nmm_frame_state_super(state1));
    cass_cond(dp != NULL);

    seq = imm_seq_create("A", abc);
    struct imm_dp_task* task = imm_dp_task_create(dp);
    imm_dp_task_setup(task, seq);
    struct imm_result const* r = imm_dp_viterbi(dp, task);
    imm_dp_task_destroy(task);
    cass_cond(r != NULL);
    imm_float loglik = imm_hmm_loglikelihood(hmm, seq, imm_result_path(r));
    cass_close(loglik, -6.0198640216);
    cass_close(imm_hmm_loglikelihood(hmm, seq, imm_result_path(r)), -6.0198640216);
    imm_seq_destroy(seq);
    imm_result_destroy(r);

    struct dcp_output* output = dcp_output_create(TMPDIR "/two_profiles.deciphon");
    cass_cond(output != NULL);

    /* First profile */
    struct nmm_profile* p = nmm_profile_create(abc);
    nmm_profile_append_model(p, imm_model_create(hmm, dp));
    nmm_profile_append_model(p, imm_model_create(hmm, dp));
    cass_equal(dcp_output_write(output, p), 0);
    nmm_profile_destroy(p, false);

    /* Second profile */
    p = nmm_profile_create(abc);
    nmm_profile_append_model(p, imm_model_create(hmm, dp));
    nmm_profile_append_model(p, imm_model_create(hmm, dp));
    cass_equal(dcp_output_write(output, p), 0);
    nmm_profile_destroy(p, false);

    cass_equal(dcp_output_destroy(output), 0);

    nmm_base_abc_destroy(base);
    nmm_codon_destroy(codon);
    imm_hmm_destroy(hmm);
    nmm_frame_state_destroy(state1);
    nmm_codon_state_destroy(state2);
    nmm_codon_marg_destroy(codont);
    nmm_base_lprob_destroy(basep);
    imm_dp_destroy(dp);
    nmm_codon_lprob_destroy(codonp);
}

void test_input(void)
{
    struct dcp_input* input = dcp_input_create(TMPDIR "/two_profiles.deciphon");
    cass_cond(input != NULL);

    cass_cond(!dcp_input_end(input));

    /* ------------------ first profile ------------------ */
    struct dcp_profile const* prof = dcp_input_read(input);
    cass_cond(!dcp_input_end(input));
    cass_cond(prof != NULL);
    cass_equal(dcp_profile_nmodels(prof), 2);
    struct imm_abc const* abc = dcp_profile_abc(prof);

    struct imm_model* model = dcp_profile_get_model(prof, 0);

    cass_equal(imm_model_nstates(model), 2);

    struct imm_hmm*      hmm = imm_model_hmm(model);
    struct imm_dp const* dp = imm_model_dp(model);

    struct imm_seq const* seq = imm_seq_create("A", abc);
    struct imm_dp_task*   task = imm_dp_task_create(dp);
    imm_dp_task_setup(task, seq);
    struct imm_result const* r = imm_dp_viterbi(dp, task);
    cass_cond(r != NULL);
    imm_float loglik = imm_hmm_loglikelihood(hmm, seq, imm_result_path(r));

    cass_close(loglik, -6.0198640216);
    cass_close(imm_hmm_loglikelihood(hmm, seq, imm_result_path(r)), -6.0198640216);
    imm_result_destroy(r);

    imm_dp_task_destroy(task);
    imm_seq_destroy(seq);
    dcp_profile_destroy(prof, true);

    /* ------------------ second profile ------------------ */
    prof = dcp_input_read(input);
    cass_cond(prof != NULL);
    cass_cond(dcp_input_end(input));
    cass_equal(dcp_profile_nmodels(prof), 2);
    abc = dcp_profile_abc(prof);

    /* First model */
    model = dcp_profile_get_model(prof, 0);

    cass_equal(imm_model_nstates(model), 2);

    hmm = imm_model_hmm(model);
    dp = imm_model_dp(model);

    seq = imm_seq_create("A", abc);
    task = imm_dp_task_create(dp);
    imm_dp_task_setup(task, seq);
    r = imm_dp_viterbi(dp, task);
    cass_cond(r != NULL);
    loglik = imm_hmm_loglikelihood(hmm, seq, imm_result_path(r));

    cass_close(loglik, -6.0198640216);
    cass_close(imm_hmm_loglikelihood(hmm, seq, imm_result_path(r)), -6.0198640216);
    imm_result_destroy(r);

    imm_dp_task_destroy(task);
    imm_seq_destroy(seq);

    /* Second model */
    model = dcp_profile_get_model(prof, 1);

    cass_equal(imm_model_nstates(model), 2);

    hmm = imm_model_hmm(model);
    dp = imm_model_dp(model);

    seq = imm_seq_create("A", abc);
    task = imm_dp_task_create(dp);
    imm_dp_task_setup(task, seq);
    r = imm_dp_viterbi(dp, task);
    cass_cond(r != NULL);
    loglik = imm_hmm_loglikelihood(hmm, seq, imm_result_path(r));

    cass_close(loglik, -6.0198640216);
    cass_close(imm_hmm_loglikelihood(hmm, seq, imm_result_path(r)), -6.0198640216);
    imm_result_destroy(r);

    imm_dp_task_destroy(task);
    imm_seq_destroy(seq);
    dcp_profile_destroy(prof, true);
    dcp_input_destroy(input);
}

void test_small(void)
{
    struct dcp_server* server = dcp_server_create(TMPDIR "/two_profiles.deciphon", "A");
    dcp_server_start(server);
    uint32_t                  nresults = 0;
    struct dcp_result const** results = dcp_server_results(server, &nresults);
    dcp_server_destroy(server);

    for (uint32_t i = 0; i < nresults; ++i) {
        printf("%d %.10f %.10f\n", dcp_result_profid(results[i]), dcp_result_null_loglik(results[i]),
               dcp_result_alt_loglik(results[i]));
    }

    for (uint32_t i = 0; i < nresults; ++i)
        free((void*)results[i]);
    free(results);
}

void test_pfam(void)
{
    char const seq[] = "AAAACGCGTGTCACGACAACGCGTACGTTTCGACGAGTACGACGCCCGGG"
                       "AAAACGCGTGTCGACGACGAACGCGTACGTTTACGACGAGTACGACGCCC"
                       "AAAACGCGTGTCACGACAACGCGTACGTTTCGACGAGTACGACGCCCGGG"
                       "AAAACGCGTGTCGACGACGAACGCGTACGTTTACGACGAGTACGACGCCC"
                       "AAAACGCGTGTCACGACAACGCGTACGTTTCGACGAGTACGACGCCCGGG"
                       "AAAACGCGTGTCGACGACGAACGCGTACGTTTACGACGAGTACGACGCCC"
                       "AAAACGCGTGTCACGACAACGCGTACGTTTCGACGAGTACGACGCCCGGG"
                       "AAAACGCGTGTCGACGACGAACGCGTACGTTTACGACGAGTACGACGCCC"
                       "AAAACGCGTGTCACGACAACGCGTACGTTTCGACGAGTACGACGCCCGGG"
                       "AAAACGCGTGTCGACGACGAACGCGTACGTTTACGACGAGTACGACGCCC"
                       "AAAACGCGTGTCACGACAACGCGTACGTTTCGACGAGTACGACGCCCGGG"
                       "AAAACGCGTGTCGACGACGAACGCGTACGTTTACGACGAGTACGACGCCC"
                       "AAAACGCGTGTCACGACAACGCGTACGTTTCGACGAGTACGACGCCCGGG"
                       "AAAACGCGTGTCGACGACGAACGCGTACGTTTACGACGAGTACGACGCCC"
                       "AAAACGCGTGTCACGACAACGCGTACGTTTCGACGAGTACGACGCCCGGG"
                       "AAAACGCGTGTCGACGACGAACGCGTACGTTTACGACGAGTACGACGCCC"
                       "AAAACGCGTGTCACGACAACGCGTACGTTTCGACGAGTACGACGCCCGGG"
                       "AAAACGCGTGTCGACGACGAACGCGTACGTTTACGACGAGTACGACGCCC"
                       "AAAACGCGTGTCACGACAACGCGTACGTTTCGACGAGTACGACGCCCGGG"
                       "AAAACGCGTGTCGACGACGAACGCGTACGTTTACGACGAGTACGACGCCC"
                       "AAAACGCGTGTCACGACAACGCGTACGTTTCGACGAGTACGACGCCCGGG"
                       "AAAACGCGTGTCGACGACGAACGCGTACGTTTACGACGAGTACGACGCCC"
                       "AAAACGCGTGTCACGACAACGCGTACGTTTCGACGAGTACGACGCCCGGG"
                       "AAAACGCGTGTCGACGACGAACGCGTACGTTTACGACGAGTACGACGCCC"
                       "AAAACGCGTGTCACGACAACGCGTACGTTTCGACGAGTACGACGCCCGGG"
                       "AAAACGCGTGTCGACGACGAACGCGTACGTTTACGACGAGTACGACGCCC"
                       "AAAACGCGTGTCACGACAACGCGTACGTTTCGACGAGTACGACGCCCGGG"
                       "AAAACGCGTGTCGACGACGAACGCGTACGTTTACGACGAGTACGACGCCC"
                       "AAAACGCGTGTCACGACAACGCGTACGTTTCGACGAGTACGACGCCCGGG"
                       "AAAACGCGTGTCGACGACGAACGCGTACGTTTACGACGAGTACGACGCCC"
                       "AAAACGCGTGTCACGACAACGCGTACGTTTCGACGAGTACGACGCCCGGG"
                       "AAAACGCGTGTCGACGACGAACGCGTACGTTTACGACGAGTACGACGCCC"
                       "AAAACGCGTGTCACGACAACGCGTACGTTTCGACGAGTACGACGCCCGGG"
                       "AAAACGCGTGTCGACGACGAACGCGTACGTTTACGACGAGTACGACGCCC"
                       "AAAACGCGTGTCACGACAACGCGTACGTTTCGACGAGTACGACGCCCGGG"
                       "AAAACGCGTGTCGACGACGAACGCGTACGTTTACGACGAGTACGACGCCC"
                       "AAAACGCGTGTCACGACAACGCGTACGTTTCGACGAGTACGACGCCCGGG"
                       "AAAACGCGTGTCGACGACGAACGCGTACGTTTACGACGAGTACGACGCCC"
                       "AAAACGCGTGTCACGACAACGCGTACGTTTCGACGAGTACGACGCCCGGG"
                       "AAAACGCGTGTCGACGACGAACGCGTACGTTTACGACGAGTACGACGCCC";

    unsigned long N = 1;
    /* unsigned long N = 30; */
    char* s = malloc(sizeof(char) * (N * 2000 + 1));
    for (unsigned long i = 0; i < N * 2000; ++i) {
        s[i] = seq[i % 2000];
    }
    s[N * 2000] = '\0';

    cass_cond(strlen(s) == N * 2000);
    struct dcp_server* server = dcp_server_create("/Users/horta/tmp/deciphon/Pfam-A.deciphon", s);
    dcp_server_start(server);
    dcp_server_destroy(server);
}

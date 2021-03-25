#include "cass/cass.h"
#include "dcp/dcp.h"
#include "elapsed/elapsed.h"
#include "helper.h"
#include "nmm/nmm.h"
#include <pthread.h>

#ifndef TMPDIR
#define TMPDIR ""
#endif

void test_output(void);
void test_input(void);
void test_small(bool calc_loglik, bool calc_null, bool multiple_hits, bool hmmer3_compat);
void test_pfam(void);

int main(void)
{
    test_output();
    test_input();
    test_small(true, true, true, true);
    /* test_small(false, true, true, true); */
    /* test_small(true, false, true, true); */
    /* test_small(false, false, true, true); */
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

    struct imm_dp const* dp = imm_hmm_create_dp(hmm, nmm_codon_state_super(state2));
    cass_cond(dp != NULL);

    seq = imm_seq_create("ACCATG", abc);
    struct imm_dp_task* task = imm_dp_task_create(dp);
    imm_dp_task_setup(task, seq);
    struct imm_result const* r = imm_dp_viterbi(dp, task);
    imm_dp_task_destroy(task);
    cass_cond(r != NULL);
    imm_float loglik = imm_hmm_loglikelihood(hmm, seq, imm_result_path(r));
    cass_close(loglik, -10.0458194611);
    cass_close(imm_hmm_loglikelihood(hmm, seq, imm_result_path(r)), -10.0458194611);
    imm_seq_destroy(seq);
    imm_result_destroy(r);

    struct dcp_output* output = dcp_output_create(TMPDIR "/two_profiles.dcp");
    cass_cond(output != NULL);

    /* First profile */
    struct dcp_metadata const* mt = dcp_metadata_create("name0", "acc0");
    struct dcp_profile*        p = dcp_profile_create(abc, mt);
    dcp_profile_add_model(p, imm_model_create(hmm, dp));
    dcp_profile_add_model(p, imm_model_create(hmm, dp));
    cass_equal(dcp_output_write(output, p), 0);
    dcp_metadata_destroy(mt);
    dcp_profile_destroy(p, false);
    imm_dp_destroy(dp);

    /* Second profile */
    imm_hmm_set_trans(hmm, nmm_frame_state_super(state1), nmm_codon_state_super(state2), imm_log(0.9));
    dp = imm_hmm_create_dp(hmm, nmm_codon_state_super(state2));
    mt = dcp_metadata_create("name1", "acc1");
    p = dcp_profile_create(abc, mt);
    dcp_profile_add_model(p, imm_model_create(hmm, dp));
    dcp_profile_add_model(p, imm_model_create(hmm, dp));
    cass_equal(dcp_output_write(output, p), 0);
    dcp_metadata_destroy(mt);
    dcp_profile_destroy(p, false);
    imm_dp_destroy(dp);

    cass_equal(dcp_output_destroy(output), 0);

    nmm_base_abc_destroy(base);
    nmm_codon_destroy(codon);
    imm_hmm_destroy(hmm);
    nmm_frame_state_destroy(state1);
    nmm_codon_state_destroy(state2);
    nmm_codon_marg_destroy(codont);
    nmm_base_lprob_destroy(basep);
    nmm_codon_lprob_destroy(codonp);
}

void test_input(void)
{
    struct dcp_input* input = dcp_input_create(TMPDIR "/two_profiles.dcp");
    cass_cond(input != NULL);

    cass_cond(!dcp_input_end(input));

    /* ------------------ first profile ------------------ */
    struct dcp_profile const* prof = dcp_input_read(input);
    cass_cond(!dcp_input_end(input));
    cass_cond(prof != NULL);
    cass_equal(dcp_profile_nmodels(prof), 2);
    struct imm_abc const*      abc = dcp_profile_abc(prof);
    struct dcp_metadata const* mt = dcp_profile_metadata(prof);
    cass_equal(strncmp(dcp_metadata_acc(mt), "acc0", 4), 0);
    cass_equal(strncmp(dcp_metadata_name(mt), "name0", 5), 0);

    struct imm_model* model = dcp_profile_model(prof, 0);

    cass_equal(imm_model_nstates(model), 2);

    struct imm_hmm*      hmm = imm_model_hmm(model);
    struct imm_dp const* dp = imm_model_dp(model);

    struct imm_seq const* seq = imm_seq_create("ACCATG", abc);
    struct imm_dp_task*   task = imm_dp_task_create(dp);
    imm_dp_task_setup(task, seq);
    struct imm_result const* r = imm_dp_viterbi(dp, task);
    cass_cond(r != NULL);
    imm_float loglik = imm_hmm_loglikelihood(hmm, seq, imm_result_path(r));

    cass_close(loglik, -10.0458194611);
    cass_close(imm_hmm_loglikelihood(hmm, seq, imm_result_path(r)), -10.0458194611);
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
    mt = dcp_profile_metadata(prof);
    cass_equal(strncmp(dcp_metadata_acc(mt), "acc1", 4), 0);
    cass_equal(strncmp(dcp_metadata_name(mt), "name1", 5), 0);

    /* First model */
    model = dcp_profile_model(prof, 0);

    cass_equal(imm_model_nstates(model), 2);

    hmm = imm_model_hmm(model);
    dp = imm_model_dp(model);

    seq = imm_seq_create("ACCATG", abc);
    task = imm_dp_task_create(dp);
    imm_dp_task_setup(task, seq);
    r = imm_dp_viterbi(dp, task);
    cass_cond(r != NULL);
    loglik = imm_hmm_loglikelihood(hmm, seq, imm_result_path(r));

    cass_close(loglik, -8.5417420643);
    cass_close(imm_hmm_loglikelihood(hmm, seq, imm_result_path(r)), -8.5417420643);
    imm_result_destroy(r);

    imm_dp_task_destroy(task);
    imm_seq_destroy(seq);

    /* Second model */
    model = dcp_profile_model(prof, 1);

    cass_equal(imm_model_nstates(model), 2);

    hmm = imm_model_hmm(model);
    dp = imm_model_dp(model);

    seq = imm_seq_create("ACCATG", abc);
    task = imm_dp_task_create(dp);
    imm_dp_task_setup(task, seq);
    r = imm_dp_viterbi(dp, task);
    cass_cond(r != NULL);
    loglik = imm_hmm_loglikelihood(hmm, seq, imm_result_path(r));

    cass_close(loglik, -8.5417420643);
    cass_close(imm_hmm_loglikelihood(hmm, seq, imm_result_path(r)), -8.5417420643);
    imm_result_destroy(r);

    imm_dp_task_destroy(task);
    imm_seq_destroy(seq);
    dcp_profile_destroy(prof, true);
    dcp_input_destroy(input);
}

void test_small(bool calc_loglik, bool calc_null, bool multiple_hits, bool hmmer3_compat)
{
    char const* names[] = {"name0", "name1"};
    char const* accs[] = {"acc0", "acc1"};

    imm_float  alt_logliks[] = {imm_lprob_invalid(), -5.0748538664, -6.6237706587,
                               imm_lprob_invalid(), -3.5707764696, -5.1196932619};
    imm_float  null_logliks[] = {imm_lprob_invalid(), -5.0748538664, -6.6237706587,
                                imm_lprob_invalid(), -3.5707764696, -5.1196932619};
    imm_float* logliks[] = {[DCP_MODEL_ALT] = alt_logliks, [DCP_MODEL_NULL] = null_logliks};

    char const*  alt_path[] = {"", "S0:2,S1:3", "S0:3,S1:3", "", "S0:2,S1:3", "S0:3,S1:3"};
    char const*  null_path[] = {"", "S0:2,S1:3", "S0:3,S1:3", "", "S0:2,S1:3", "S0:3,S1:3"};
    char const** paths[] = {[DCP_MODEL_ALT] = alt_path, [DCP_MODEL_NULL] = null_path};

    struct dcp_server*  server = dcp_server_create(TMPDIR "/two_profiles.dcp");
    struct dcp_task_cfg cfg = {calc_loglik, calc_null, multiple_hits, hmmer3_compat, false};
    struct dcp_task*    task = dcp_task_create(cfg);

    dcp_task_add_seq(task, "ACT");
    dcp_task_add_seq(task, "AGATG");
    dcp_task_add_seq(task, "CCCCCC");

    dcp_server_add(server, task);
    dcp_server_start(server);

    unsigned ncomp = 0;
    while (!dcp_task_end(task)) {

        struct dcp_results* results = dcp_task_read(task);
        if (!results)
            continue;

        for (uint16_t i = 0; i < dcp_results_size(results); ++i) {
            struct dcp_result const* r = dcp_results_get(results, i);
            uint32_t                 seqid = dcp_result_seqid(r);
            uint32_t                 profid = dcp_result_profid(r);

            for (unsigned j = 0; j < ARRAY_SIZE(dcp_models); ++j) {

                enum dcp_model m = dcp_models[j];
                unsigned       k = 3 * profid + seqid;

                cass_close(dcp_result_loglik(r, m), logliks[m][k]);

                struct dcp_metadata const* mt = dcp_server_metadata(server, profid);
                cass_equal(strcmp(dcp_metadata_name(mt), names[profid]), 0);
                cass_equal(strcmp(dcp_metadata_acc(mt), accs[profid]), 0);

                struct dcp_string const* path = dcp_result_path(r, m);
                cass_equal(strcmp(dcp_string_data(path), paths[m][k]), 0);

                ncomp++;
            }
        }
        dcp_server_recyle(server, results);
    }
    cass_equal(ncomp, 12);
    dcp_task_destroy(task);

    dcp_server_stop(server);
    dcp_server_join(server);
    dcp_server_destroy(server);
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
    /* uint32_t nresults = 0; */
    /* struct dcp_server*        server = dcp_server_create("/Users/horta/tmp/dcp/Pfam-A.dcp"); */
    /* struct dcp_result const** results = dcp_server_scan(server, s, &nresults); */
    /* for (uint32_t i = 0; i < nresults; ++i) */
    /* dcp_result_destroy(results[i]); */
    /* free(results); */
    /* dcp_server_destroy(server); */
}

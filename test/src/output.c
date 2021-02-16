#include "cass/cass.h"
#include "deciphon/deciphon.h"
#include "nmm/nmm.h"
#include "imm/imm.h"
#include "helper.h"

#ifndef TMPDIR
#define TMPDIR ""
#endif

void test_output(void);

int main(void)
{
    test_output();
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

    struct nmm_frame_state const* state1 =
        nmm_frame_state_create("S0", basep, codont, (imm_float)0.1);
    struct nmm_codon_state const* state2 = nmm_codon_state_create("S1", codonp);

    imm_hmm_add_state(hmm, nmm_frame_state_super(state1), imm_log(1.0));
    imm_hmm_add_state(hmm, nmm_codon_state_super(state2), imm_log(0.0001));
    imm_hmm_set_trans(hmm, nmm_frame_state_super(state1), nmm_codon_state_super(state2),
                      imm_log(0.2));

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
    imm_dp_task_setup(task, seq, 0);
    struct imm_results const* results = imm_dp_viterbi(dp, task);
    imm_dp_task_destroy(task);
    cass_cond(results != NULL);
    cass_cond(imm_results_size(results) == 1);
    struct imm_result const* r = imm_results_get(results, 0);
    struct imm_subseq        subseq = imm_result_subseq(r);
    struct imm_path const*   p = imm_result_path(r);
    imm_float                loglik = imm_hmm_loglikelihood(hmm, imm_subseq_cast(&subseq), p);
    cass_close(loglik, -6.0198640216);
    cass_close(imm_hmm_loglikelihood(hmm, seq, imm_result_path(r)), -6.0198640216);
    imm_seq_destroy(seq);
    imm_results_destroy(results);

    struct dcp_output* output = dcp_output_create(TMPDIR "/three_models.deciphon", 3);
    cass_cond(output != NULL);

    struct nmm_model* m = nmm_model_create(abc);
    nmm_model_append_hmm_block(m, imm_hmm_block_create(hmm, dp));
    cass_equal(dcp_output_write(output, m), 0);
    cass_equal(dcp_output_write(output, m), 0);
    cass_equal(dcp_output_write(output, m), 0);
    nmm_model_destroy(m);
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

    struct dcp_input* input = dcp_input_create(TMPDIR "/three_models.deciphon");
    cass_cond(input != NULL);
    struct dcp_partition *part = dcp_input_create_partition(input, 0, 2);
    cass_equal(dcp_partition_nmodels(part), 2);
    cass_cond(!dcp_partition_eof(part));
    struct nmm_model const* model = dcp_partition_read(part);
    cass_cond(!dcp_partition_eof(part));
    cass_cond(model != NULL);
    nmm_model_destroy(model);
    model = dcp_partition_read(part);
    cass_cond(dcp_partition_eof(part));
    cass_cond(model != NULL);

    dcp_input_destroy(input);

    struct imm_hmm_block* block = nmm_model_get_hmm_block(model, 0);

    cass_equal(imm_hmm_block_nstates(block), 2);

    abc = nmm_model_abc(model);
    hmm = imm_hmm_block_hmm(block);
    dp = imm_hmm_block_dp(block);

    seq = imm_seq_create("A", abc);
    task = imm_dp_task_create(dp);
    imm_dp_task_setup(task, seq, 0);
    results = imm_dp_viterbi(dp, task);
    cass_cond(results != NULL);
    cass_cond(imm_results_size(results) == 1);
    r = imm_results_get(results, 0);
    subseq = imm_result_subseq(r);
    p = imm_result_path(r);
    loglik = imm_hmm_loglikelihood(hmm, imm_subseq_cast(&subseq), p);
    cass_close(loglik, -6.0198640216);
    cass_close(imm_hmm_loglikelihood(hmm, seq, imm_result_path(r)), -6.0198640216);
    imm_results_destroy(results);

    for (uint16_t i = 0; i < imm_hmm_block_nstates(block); ++i)
        imm_state_destroy(imm_hmm_block_state(block, i));

    for (uint16_t i = 0; i < nmm_model_nbase_lprobs(model); ++i)
        nmm_base_lprob_destroy(nmm_model_base_lprob(model, i));

    for (uint16_t i = 0; i < nmm_model_ncodon_margs(model); ++i)
        nmm_codon_marg_destroy(nmm_model_codon_marg(model, i));

    for (uint16_t i = 0; i < nmm_model_ncodon_lprobs(model); ++i)
        nmm_codon_lprob_destroy(nmm_model_codon_lprob(model, i));

    imm_seq_destroy(seq);
    imm_abc_destroy(abc);
    imm_hmm_destroy(hmm);
    imm_dp_destroy(dp);
    nmm_model_destroy(model);
    imm_dp_task_destroy(task);
}

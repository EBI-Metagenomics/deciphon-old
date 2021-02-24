#include "ck_ring.h"
#include "deciphon/deciphon.h"
#include "elapsed/elapsed.h"
#include "nmm/nmm.h"
#include <stdatomic.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#define BUFFSIZE 256

void process(char const* seq_str, struct nmm_profile const* prof);

int dcp_master(char const* db_filepath, char const* seq_str)
{
    struct dcp_input*   input = dcp_input_create(db_filepath);
    ck_ring_t ring_spmc CK_CC_CACHELINE;
    ck_ring_buffer_t*   buffer = malloc(sizeof(*buffer) * BUFFSIZE);
    memset(buffer, 0, sizeof(*buffer) * BUFFSIZE);
    ck_ring_init(&ring_spmc, BUFFSIZE);

    atomic_bool    finished = false;
    struct elapsed elapsed = elapsed_init();
    elapsed_start(&elapsed);
#pragma omp parallel default(none) shared(input, seq_str, ring_spmc, buffer, finished)
    {
#pragma omp master
        {
            while (!dcp_input_end(input)) {
                struct nmm_profile const* prof = dcp_input_read(input);

                while (!ck_ring_enqueue_spmc(&ring_spmc, buffer, prof))
                    ck_pr_stall();
            }
            finished = true;
        }

        while (true) {
            struct nmm_profile const* prof = NULL;

            while (!ck_ring_dequeue_spmc(&ring_spmc, buffer, &prof) && !finished)
                ck_pr_stall();

            if (!prof && finished)
                break;

            process(seq_str, prof);
            nmm_profile_destroy(prof, true);
        }
    }
    dcp_input_destroy(input);
    free(buffer);
    elapsed_end(&elapsed);
    printf("Elapsed time: %f seconds\n", elapsed_seconds(&elapsed));
    return 0;
}

void process(char const* seq_str, struct nmm_profile const* prof)
{
    struct imm_abc const* abc = nmm_profile_abc(prof);
    struct imm_seq const* seq = imm_seq_create(seq_str, abc);

    struct imm_model* alt = nmm_profile_get_model(prof, 0);
    struct imm_model* null = nmm_profile_get_model(prof, 1);

    struct imm_hmm* hmm_alt = imm_model_hmm(alt);
    struct imm_hmm* hmm_null = imm_model_hmm(null);

    struct imm_dp const* dp_alt = imm_model_dp(alt);
    struct imm_dp const* dp_null = imm_model_dp(null);

    struct imm_dp_task* task_alt = imm_dp_task_create(dp_alt);
    struct imm_dp_task* task_null = imm_dp_task_create(dp_null);

    imm_dp_task_setup(task_alt, seq, 0);
    struct imm_results const* results = imm_dp_viterbi(dp_alt, task_alt);
    struct imm_result const*  r = imm_results_get(results, 0);
    struct imm_subseq         subseq = imm_result_subseq(r);
    imm_float                 alt_loglik = imm_hmm_loglikelihood(hmm_alt, imm_subseq_cast(&subseq), imm_result_path(r));
    imm_results_destroy(results);

    imm_dp_task_setup(task_null, seq, 0);
    results = imm_dp_viterbi(dp_null, task_null);
    r = imm_results_get(results, 0);
    subseq = imm_result_subseq(r);
    imm_float null_loglik = imm_hmm_loglikelihood(hmm_null, imm_subseq_cast(&subseq), imm_result_path(r));
    imm_results_destroy(results);

    printf("alt_loglik: %f vs null_loglik: %f\n", alt_loglik, null_loglik);

    imm_seq_destroy(seq);
    imm_dp_task_destroy(task_alt);
    imm_dp_task_destroy(task_null);
}

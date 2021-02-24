#include "deciphon/deciphon.h"
#include "nmm/nmm.h"
#include "queue.h"
#include "task.h"

#ifdef _OPENMP
#include <omp.h>
#endif

void process(char const* seq_str, struct nmm_profile const* prof);

int dcp_master(char const* db_filepath, char const* seq_str)
{
    struct dcp_input* input = dcp_input_create(db_filepath);
    struct queue*     queue = queue_create(256);

#pragma omp parallel default(none) shared(input, seq_str, queue)
    {
#pragma omp master
        {
            /* int i = 0; */
            while (!dcp_input_end(input)) {
                /* printf("Producer %d\n", i++); */
                struct nmm_profile const* prof = dcp_input_read(input);
                struct task*              task = task_create(prof);
                queue_push(queue, task);
            }
            queue_finish(queue);
        }

        while (true) {
            struct task* task = NULL;

            task = queue_pop(queue);
            if (task) {
                /* printf("Process\n"); */
                process(seq_str, task->profile);
            } else {
                break;
            }
        }
    }
    queue_destroy(queue);
    dcp_input_destroy(input);
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

    /* printf("alt_loglik: %f vs null_loglik: %f\n", alt_loglik, null_loglik); */

    imm_seq_destroy(seq);
    imm_dp_task_destroy(task_alt);
    imm_dp_task_destroy(task_null);
    nmm_profile_destroy(prof, true);
}

#include "deciphon/deciphon.h"
#include "elapsed/elapsed.h"
#include "nmm/nmm.h"
#ifdef OPENMP
#include <omp.h>
#endif

int dcp_master(char const* db_filepath, char const* seq_str)
{
    struct dcp_input*     input = dcp_input_create(db_filepath);
    struct dcp_partition* part = dcp_input_create_partition(input, 0, 1);
    int                   nprofs = (int)dcp_partition_nprofiles(part);

#pragma omp parallel default(none) shared(nprofs, part, seq_str) /* if(1 == 2) */
    {
#pragma omp for ordered(1)
        for (int i = 0; i < nprofs; ++i) {

#pragma omp ordered depend(sink : i - 1)
            struct nmm_profile const* prof = dcp_partition_read(part);
            printf("Inside %d\n", i);
#pragma omp ordered depend(source)

            elapsed_sleep(1);
            struct imm_abc const* abc = nmm_profile_abc(prof);
            struct imm_seq const* seq = imm_seq_create(seq_str, abc);

            struct imm_model*    alt = nmm_profile_get_model(prof, 0);
            struct imm_model*    null = nmm_profile_get_model(prof, 1);
            struct imm_hmm*      hmm_alt = imm_model_hmm(alt);
            struct imm_dp const* dp_alt = imm_model_dp(alt);
            struct imm_hmm*      hmm_null = imm_model_hmm(null);
            struct imm_dp const* dp_null = imm_model_dp(null);

            struct imm_dp_task* task_alt = imm_dp_task_create(dp_alt);
            imm_dp_task_setup(task_alt, seq, 0);
            struct imm_results const* results = imm_dp_viterbi(dp_alt, task_alt);
            /* printf("#Results: %d\n", imm_results_size(results)); */
            struct imm_result const* r = imm_results_get(results, 0);
            struct imm_subseq        subseq = imm_result_subseq(r);
            imm_float loglik = imm_hmm_loglikelihood(hmm_alt, imm_subseq_cast(&subseq), imm_result_path(r));
            printf("Outside %d, loglik: %f\n", i, loglik);

            nmm_profile_destroy(prof);
        }
    }
    dcp_partition_destroy(part);
    dcp_input_destroy(input);
    return 0;
}

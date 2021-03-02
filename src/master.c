#include "ck_ring.h"
#include "deciphon/deciphon.h"
#include "elapsed/elapsed.h"
#include "nmm/nmm.h"
#include "profile.h"
#include <stdatomic.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#define BUFFSIZE 256

void process(char const* seq_str, struct dcp_profile const* prof);

int dcp_master(char const* db_filepath, char const* seq_str)
{
    struct dcp_input* input = dcp_input_create(db_filepath);

    ck_ring_t ring_spmc CK_CC_CACHELINE;
    ck_ring_init(&ring_spmc, BUFFSIZE);
    ck_ring_buffer_t buffer[BUFFSIZE] = {0};

    atomic_bool    finished = false;
    struct elapsed elapsed = elapsed_init();
    elapsed_start(&elapsed);
#pragma omp parallel default(none) shared(input, seq_str, ring_spmc, buffer, finished)
    {
#pragma omp master
        {
            while (!dcp_input_end(input)) {
                struct dcp_profile const* prof = dcp_input_read(input);
                printf("dcp_input_read\n");

                while (!ck_ring_enqueue_spmc(&ring_spmc, buffer, prof))
                    ck_pr_stall();
            }
            finished = true;
        }

        bool ok = true;
        do {
            struct dcp_profile const* prof = NULL;

            while (!(ok = ck_ring_dequeue_spmc(&ring_spmc, buffer, &prof)) && !finished) {
                ck_pr_stall();
            }

            if (ok) {
                process(seq_str, prof);
                dcp_profile_destroy(prof, true);
                printf("dcp_profile_destroy\n");
            }
        } while (ok || !finished);
    }
    dcp_input_destroy(input);
    elapsed_end(&elapsed);
    printf("Elapsed time: %f seconds\n", elapsed_seconds(&elapsed));
    return 0;
}

void process(char const* seq_str, struct dcp_profile const* prof)
{
    struct nmm_profile const* p = profile_nmm(prof);
    struct imm_abc const*     abc = nmm_profile_abc(p);
    struct imm_seq const*     seq = imm_seq_create(seq_str, abc);

    struct imm_model* alt = nmm_profile_get_model(p, 0);
    struct imm_model* null = nmm_profile_get_model(p, 1);

    struct imm_hmm* hmm_alt = imm_model_hmm(alt);
    struct imm_hmm* hmm_null = imm_model_hmm(null);

    struct imm_dp const* dp_alt = imm_model_dp(alt);
    struct imm_dp const* dp_null = imm_model_dp(null);

    struct imm_dp_task* task_alt = imm_dp_task_create(dp_alt);
    struct imm_dp_task* task_null = imm_dp_task_create(dp_null);

    imm_dp_task_setup(task_alt, seq);
    struct imm_result const* r = imm_dp_viterbi(dp_alt, task_alt);
    imm_float                alt_loglik = imm_hmm_loglikelihood(hmm_alt, seq, imm_result_path(r));
    imm_result_destroy(r);

    imm_dp_task_setup(task_null, seq);
    r = imm_dp_viterbi(dp_null, task_null);
    imm_float null_loglik = imm_hmm_loglikelihood(hmm_null, seq, imm_result_path(r));
    imm_result_destroy(r);

    imm_seq_destroy(seq);
    imm_dp_task_destroy(task_alt);
    imm_dp_task_destroy(task_null);
}

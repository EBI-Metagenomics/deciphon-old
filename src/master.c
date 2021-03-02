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

struct dcp_result
{
    uint32_t                 profid;
    struct imm_result* const null_result;
    struct imm_result* const alt_result;
};

struct profile_ring
{
    ck_ring_t ring   CK_CC_CACHELINE;
    ck_ring_buffer_t buffer[BUFFSIZE];
};

void profile_ring_init(struct profile_ring* ring);
void profile_ring_push(struct profile_ring* ring, struct dcp_profile const* prof);
struct dcp_profile const* profile_ring_pop(struct profile_ring* ring);

void profile_ring_init(struct profile_ring* ring)
{
    ck_ring_init(&ring->ring, BUFFSIZE);
    memset(ring->buffer, 0, IMM_ARRAY_SIZE(ring->buffer));
}

void profile_ring_push(struct profile_ring* ring, struct dcp_profile const* prof)
{
    while (!ck_ring_enqueue_spmc(&ring->ring, ring->buffer, prof))
        ck_pr_stall();
}

struct dcp_profile const* profile_ring_pop(struct profile_ring* ring)
{
    struct dcp_profile const* prof = NULL;
    bool ok = ck_ring_dequeue_spmc(&ring->ring, ring->buffer, &prof);
    if (ok)
        return prof;
    ck_pr_stall();
    return NULL;
}

/* struct result_ring */
/* { */

/* }; */

void process(char const* seq_str, struct dcp_profile const* prof);

int dcp_master(char const* db_filepath, char const* seq_str)
{
    struct dcp_input* input = dcp_input_create(db_filepath);

    struct profile_ring prof_ring;
    profile_ring_init(&prof_ring);

    atomic_bool    finished = false;
    struct elapsed elapsed = elapsed_init();
    elapsed_start(&elapsed);
#pragma omp parallel default(none) shared(input, seq_str, prof_ring, finished)
    {
#pragma omp master
        {
            while (!dcp_input_end(input)) {
                struct dcp_profile const* prof = dcp_input_read(input);
                printf("dcp_input_read\n");

                profile_ring_push(&prof_ring, prof);
            }
            finished = true;
        }

        struct dcp_profile const* prof = NULL;
        do {
            while (!(prof = profile_ring_pop(&prof_ring)) && !finished);

            if (prof) {
                process(seq_str, prof);
                dcp_profile_destroy(prof, true);
                printf("dcp_profile_destroy\n");
            }
        } while (prof || !finished);
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

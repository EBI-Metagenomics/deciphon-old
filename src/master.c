#include "ck_ring.h"
#include "deciphon/deciphon.h"
#include "elapsed/elapsed.h"
#include "free.h"
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
    imm_float                null_loglik;
    struct imm_result* const alt_result;
    imm_float                alt_loglik;
};

void dcp_result_destroy(struct dcp_result const* result);

void dcp_result_destroy(struct dcp_result const* result)
{
    imm_result_destroy(result->null_result);
    imm_result_destroy(result->alt_result);
    free_c(result);
}

struct profile_ring
{
    ck_ring_t ring   CK_CC_CACHELINE;
    ck_ring_buffer_t buffer[BUFFSIZE];
};

struct profile_ring       profile_ring_init(void);
void                      profile_ring_push(struct profile_ring* ring, struct dcp_profile const* prof);
struct dcp_profile const* profile_ring_pop(struct profile_ring* ring);

struct profile_ring profile_ring_init(void)
{
    struct profile_ring ring;
    ck_ring_init(&ring.ring, BUFFSIZE);
    memset(ring.buffer, 0, IMM_ARRAY_SIZE(ring.buffer));
    return ring;
}

void profile_ring_push(struct profile_ring* ring, struct dcp_profile const* prof)
{
    while (!ck_ring_enqueue_spmc(&ring->ring, ring->buffer, prof))
        ck_pr_stall();
}

struct dcp_profile const* profile_ring_pop(struct profile_ring* ring)
{
    struct dcp_profile const* prof = NULL;
    bool                      ok = ck_ring_dequeue_spmc(&ring->ring, ring->buffer, &prof);
    if (ok)
        return prof;
    ck_pr_stall();
    return NULL;
}

struct result_ring
{
    ck_ring_t ring   CK_CC_CACHELINE;
    ck_ring_buffer_t buffer[BUFFSIZE];
};

struct result_ring       result_ring_init(void);
void                     result_ring_push(struct result_ring* ring, struct dcp_result const* result);
struct dcp_result const* result_ring_pop(struct result_ring* ring);

struct result_ring result_ring_init(void)
{
    struct result_ring ring;
    ck_ring_init(&ring.ring, BUFFSIZE);
    memset(ring.buffer, 0, IMM_ARRAY_SIZE(ring.buffer));
    return ring;
}

void result_ring_push(struct result_ring* ring, struct dcp_result const* result)
{
    while (!ck_ring_enqueue_spsc(&ring->ring, ring->buffer, result))
        ck_pr_stall();
}

struct dcp_result const* result_ring_pop(struct result_ring* ring)
{
    struct dcp_result const* result = NULL;
    bool                     ok = ck_ring_dequeue_mpmc(&ring->ring, ring->buffer, &result);
    if (ok)
        return result;
    ck_pr_stall();
    return NULL;
}

struct dcp_result process(char const* seq_str, struct dcp_profile const* prof);

int dcp_master(char const* db_filepath, char const* seq_str)
{
    struct dcp_input* input = dcp_input_create(db_filepath);

    struct profile_ring prof_ring = profile_ring_init();
    struct result_ring  result_ring = result_ring_init();

    atomic_bool    finished = false;
    atomic_uint    running_tasks = 0;
    struct elapsed elapsed = elapsed_init();
    elapsed_start(&elapsed);
#pragma omp        parallel default(none) shared(input, seq_str, prof_ring, result_ring, finished, running_tasks)
    {
#pragma omp single nowait
        {
            printf("Single 1\n");
            while (!dcp_input_end(input)) {
                struct dcp_profile const* prof = dcp_input_read(input);
                printf("dcp_input_read\n");

                profile_ring_push(&prof_ring, prof);
            }
            finished = true;
        }

#pragma omp task
        {
            running_tasks++;
            printf("Task\n");
            struct dcp_profile const* prof = NULL;
            do {
                while (!(prof = profile_ring_pop(&prof_ring)) && !finished)
                    ;

                if (prof) {
                    struct dcp_result  r = process(seq_str, prof);
                    struct dcp_result* nr = malloc(sizeof(*nr));
                    memcpy(nr, &r, sizeof(r));
                    result_ring_push(&result_ring, nr);
                    dcp_profile_destroy(prof, true);
                    printf("dcp_profile_destroy\n");
                }
            } while (prof || !finished);
            running_tasks--;
        }

#pragma omp single nowait
        {
            printf("Single 2\n");
            struct dcp_result const* result = NULL;
            do {
                while (!(result = result_ring_pop(&result_ring)) && (!finished || running_tasks > 0))
                    ;

                if (result) {
                    printf("got new result\n");
                    dcp_result_destroy(result);
                }
            } while (result || !finished || running_tasks > 0);
        }
    }
#pragma omp barrier
    {
        dcp_input_destroy(input);
        elapsed_end(&elapsed);
        printf("Elapsed time: %f seconds\n", elapsed_seconds(&elapsed));
    }
    return 0;
}

struct dcp_result process(char const* seq_str, struct dcp_profile const* prof)
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
    struct imm_result const* alt_result = imm_dp_viterbi(dp_alt, task_alt);
    imm_float                alt_loglik = imm_hmm_loglikelihood(hmm_alt, seq, imm_result_path(alt_result));

    imm_dp_task_setup(task_null, seq);
    struct imm_result const* null_result = imm_dp_viterbi(dp_null, task_null);
    imm_float                null_loglik = imm_hmm_loglikelihood(hmm_null, seq, imm_result_path(null_result));

    imm_seq_destroy(seq);
    imm_dp_task_destroy(task_alt);
    imm_dp_task_destroy(task_null);
    return (struct dcp_result){dcp_profile_id(prof), null_result, null_loglik, alt_result, alt_loglik};
}

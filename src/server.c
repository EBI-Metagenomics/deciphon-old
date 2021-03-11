#include "deciphon/deciphon.h"
#include "nmm/nmm.h"
#include "profile.h"
#include "profile_ring.h"
#include "result.h"
#include "result_ring.h"
#include "sequence.h"
#include "task.h"
#include <stdatomic.h>

#ifdef _OPENMP
#include <omp.h>
#endif

struct dcp_server
{
    atomic_bool         finished;
    atomic_uint         nrunning_tasks;
    char const*         filepath;
    struct profile_ring profiles;
    struct dcp_input*   input;
};

void               destroy(struct dcp_server* server);
void               init(struct dcp_server* server, char const* filepath, char const* seq);
void               profile_consumer(struct dcp_server* server, struct dcp_task* task);
void               profile_producer(struct dcp_server* server);
struct dcp_result* scan(struct dcp_server* server, struct dcp_profile const* profile, char const* sequence,
                        uint32_t seqid, struct dcp_task_cfg const* cfg);

struct dcp_server* dcp_server_create(char const* filepath)
{
    struct dcp_server* server = malloc(sizeof(*server));
    server->finished = false;
    server->nrunning_tasks = 0;
    server->filepath = strdup(filepath);
    server->profiles = profile_ring_init();
    server->input = dcp_input_create(server->filepath);
    if (!server->input) {
        free((void*)server->filepath);
        free(server);
        return NULL;
    }
    return server;
}

void dcp_server_destroy(struct dcp_server const* server)
{
    free((void*)server->filepath);
    dcp_input_destroy(server->input);
    free((void*)server);
}

struct dcp_metadata const* dcp_server_metadata(struct dcp_server const* server, uint32_t profid)
{
    return dcp_input_metadata(server->input, profid);
}

void dcp_server_scan(struct dcp_server* server, struct dcp_task* task)
{
#pragma omp        parallel default(none) shared(server, task)
    {
#pragma omp single nowait
        profile_producer(server);

#pragma omp task
        profile_consumer(server, task);
    }
}

void profile_consumer(struct dcp_server* server, struct dcp_task* task)
{
    server->nrunning_tasks++;
    struct dcp_profile const* prof = NULL;
    do {
        while (!(prof = profile_ring_pop(&server->profiles)) && !server->finished) {
            ck_pr_stall();
        }

        if (prof) {
            struct sequence const* seq = task_first_sequence(task);
            uint32_t               seqid = 0;
            while (seq) {
                struct dcp_result* r = scan(server, prof, seq->sequence, seqid++, task_cfg(task));
#pragma omp critical
                task_add_result(task, r);
                seq = task_next_sequence(task, seq);
            }
            dcp_profile_destroy(prof, true);
        }
    } while (prof || !server->finished);
    server->nrunning_tasks--;
}

void profile_producer(struct dcp_server* server)
{
    dcp_input_reset(server->input);
    while (!dcp_input_end(server->input)) {
        struct dcp_profile const* prof = dcp_input_read(server->input);
        profile_ring_push(&server->profiles, prof);
    }
    server->finished = true;
}

#if 0
void result_consumer(struct dcp_server* server)
{
    struct dcp_result const* result = NULL;
    do {
        while (!(result = result_ring_pop(&server->results)) && (!server->finished || server->nrunning_tasks > 0)) {
            ck_pr_stall();
        }

        if (result) {
            result_destroy(result);
        }
    } while (result || !server->finished || server->nrunning_tasks > 0);
}
#endif

struct dcp_result* scan(struct dcp_server* server, struct dcp_profile const* profile, char const* sequence,
                        uint32_t seqid, struct dcp_task_cfg const* cfg)
{
    struct nmm_profile const* p = dcp_profile_nmm_profile(profile);
    struct imm_abc const*     abc = nmm_profile_abc(p);
    struct imm_seq const*     seq = imm_seq_create(sequence, abc);

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
    imm_float                alt_loglik = imm_lprob_invalid();
    if (!imm_path_empty(imm_result_path(alt_result)))
        alt_loglik = imm_hmm_loglikelihood(hmm_alt, seq, imm_result_path(alt_result));

    imm_dp_task_setup(task_null, seq);
    struct imm_result const* null_result = imm_dp_viterbi(dp_null, task_null);
    imm_float                null_loglik = imm_lprob_invalid();
    if (!imm_path_empty(imm_result_path(null_result)))
        null_loglik = imm_hmm_loglikelihood(hmm_null, seq, imm_result_path(null_result));

    imm_seq_destroy(seq);
    imm_dp_task_destroy(task_alt);
    imm_dp_task_destroy(task_null);

    struct dcp_result* r = malloc(sizeof(*r));
    r->profid = dcp_profile_id(profile);
    r->seqid = seqid;
    r->alt_loglik = alt_loglik;
    r->alt_result = alt_result;
    r->alt_stream = NULL;
    r->null_loglik = null_loglik;
    r->null_result = null_result;
    r->null_stream = NULL;
    c_list_init(&r->link);

    struct imm_path const* path = imm_result_path(alt_result);
    struct imm_step const* step = imm_path_first(path);
    size_t                 size = 1;
    while (step) {
        size += strlen(imm_state_get_name(imm_step_state(step))) + 3;
        step = imm_path_next(path, step);
    }
    if (size > 1)
        --size;

    char* alt_stream = malloc(sizeof(*r->alt_stream) * size);
    step = imm_path_first(path);
    size_t i = 0;
    while (step) {
        char const* name = imm_state_get_name(imm_step_state(step));
        while (*name != '\0')
            alt_stream[i++] = *(name++);

        alt_stream[i++] = ':';

        if (imm_step_seq_len(step) == 0)
            alt_stream[i++] = '0';
        else if (imm_step_seq_len(step) == 1)
            alt_stream[i++] = '1';
        else if (imm_step_seq_len(step) == 2)
            alt_stream[i++] = '2';
        else if (imm_step_seq_len(step) == 3)
            alt_stream[i++] = '3';
        else if (imm_step_seq_len(step) == 4)
            alt_stream[i++] = '4';
        else if (imm_step_seq_len(step) == 5)
            alt_stream[i++] = '5';
        else if (imm_step_seq_len(step) == 6)
            alt_stream[i++] = '6';
        else if (imm_step_seq_len(step) == 7)
            alt_stream[i++] = '7';
        else if (imm_step_seq_len(step) == 8)
            alt_stream[i++] = '8';
        else if (imm_step_seq_len(step) == 9)
            alt_stream[i++] = '9';

        alt_stream[i++] = ',';

        step = imm_path_next(path, step);
    }
    if (i > 0)
        --i;
    alt_stream[i] = '\0';
    r->alt_stream = alt_stream;

    path = imm_result_path(null_result);
    step = imm_path_first(path);
    size = 1;
    while (step) {
        size += strlen(imm_state_get_name(imm_step_state(step))) + 3;
        step = imm_path_next(path, step);
    }
    if (size > 1)
        --size;

    char* null_stream = malloc(sizeof(*r->null_stream) * size);
    step = imm_path_first(path);
    i = 0;
    while (step) {
        char const* name = imm_state_get_name(imm_step_state(step));
        while (*name != '\0')
            null_stream[i++] = *(name++);

        null_stream[i++] = ':';

        if (imm_step_seq_len(step) == 0)
            null_stream[i++] = '0';
        else if (imm_step_seq_len(step) == 1)
            null_stream[i++] = '1';
        else if (imm_step_seq_len(step) == 2)
            null_stream[i++] = '2';
        else if (imm_step_seq_len(step) == 3)
            null_stream[i++] = '3';
        else if (imm_step_seq_len(step) == 4)
            null_stream[i++] = '4';
        else if (imm_step_seq_len(step) == 5)
            null_stream[i++] = '5';
        else if (imm_step_seq_len(step) == 6)
            null_stream[i++] = '6';
        else if (imm_step_seq_len(step) == 7)
            null_stream[i++] = '7';
        else if (imm_step_seq_len(step) == 8)
            null_stream[i++] = '8';
        else if (imm_step_seq_len(step) == 9)
            null_stream[i++] = '9';

        null_stream[i++] = ',';

        step = imm_path_next(path, step);
    }
    if (i > 0)
        --i;
    null_stream[i] = '\0';
    r->null_stream = null_stream;

    return r;
}

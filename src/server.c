#include "dcp/dcp.h"
#include <pthread.h>
#include "imm/imm.h"
#include "nmm/nmm.h"
#include "profile.h"
#include "profile_ring.h"
#include "result.h"
#include "result_ring.h"
#include "results.h"
#include "sequence.h"
#include "task.h"
#include <nmm/frame_state.h>
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

struct model_scan_result
{
    imm_float                loglik;
    struct imm_result const* result;
};

static void model_scan(struct imm_hmm* hmm, struct imm_dp* dp, struct imm_seq const* seq, bool calc_loglik,
                       struct dcp_model* model);
static void profile_consumer(struct dcp_server* server, struct dcp_task* task);
static void profile_producer(struct dcp_server* server);
static void scan(struct dcp_server* server, struct nmm_profile const* profile, char const* sequence,
                 struct dcp_result* result, uint32_t seqid, struct dcp_task_cfg const* cfg);

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

uint32_t dcp_server_nprofiles(struct dcp_server const* server) { return dcp_input_nprofiles(server->input); }

void dcp_server_scan(struct dcp_server* server, struct dcp_task* task)
{
#pragma omp        parallel default(none) shared(server, task)
    {
        int a = 3;
    }
    return;
#pragma omp        parallel default(none) shared(server, task)
    {
#pragma omp single nowait
        profile_producer(server);

#pragma omp task
        profile_consumer(server, task);
    }

    printf("outside_parallel_region\n");
    fflush(stdout);
}

static void profile_consumer(struct dcp_server* server, struct dcp_task* task)
{
    server->nrunning_tasks++;
    struct dcp_profile const* prof = NULL;
    printf("enter_consumer_loop\n");
    do {
        while (!(prof = profile_ring_pop(&server->profiles)) && !server->finished) {
            ck_pr_stall();
        }

        if (prof) {
            struct sequence const* seq = task_first_seq(task);
            uint32_t               seqid = 0;

            struct dcp_results* results = NULL;
            while ((results = task_alloc_results(task)) == NULL)
                ck_pr_stall();

            while (seq) {
                struct dcp_result* r = results_next(results);
                printf("Ponto 1: %p\n", (void*)r);
                fflush(stdout);

                if (!r) {
                    printf("Ponto 2\n");
                    fflush(stdout);
                    task_push_results(task, results);
                    printf("Ponto 2.5\n");
                    while ((results = task_alloc_results(task)) == NULL)
                        ck_pr_stall();
                    printf("continue\n");
                    fflush(stdout);
                    continue;
                }
                printf("Ponto 3\n");
                fflush(stdout);

                /* result_set_profid(r, dcp_profile_id(prof)); */
                /* result_set_seqid(r, seqid); */
                seqid++;
                /* scan(server, dcp_profile_nmm_profile(prof), seq->sequence, r, seqid++, task_cfg(task)); */
                seq = task_next_seq(task, seq);
            }

            task_push_results(task, results);
            /* dcp_profile_destroy(prof, true); */
        }
    } while (prof || !server->finished);
    printf("outside_consumer_loop\n");
    fflush(stdout);
    server->nrunning_tasks--;
}

static void profile_producer(struct dcp_server* server)
{
    dcp_input_reset(server->input);
    while (!dcp_input_end(server->input)) {
        struct dcp_profile const* prof = dcp_input_read(server->input);
        profile_ring_push(&server->profiles, prof);
    }
    printf("server->finished = true\n");
    fflush(stdout);
    server->finished = true;
}

static void scan(struct dcp_server* server, struct nmm_profile const* profile, char const* sequence,
                 struct dcp_result* result, uint32_t seqid, struct dcp_task_cfg const* cfg)
{
    struct imm_abc const* abc = nmm_profile_abc(profile);
    struct imm_seq const* seq = imm_seq_create(sequence, abc);

    struct imm_hmm* hmm = imm_model_hmm(nmm_profile_get_model(profile, 0));
    struct imm_dp*  dp = imm_model_dp(nmm_profile_get_model(profile, 0));
    profile_setup(hmm, dp, cfg->multiple_hits, imm_seq_length(seq), cfg->hmmer3_compat);
    model_scan(hmm, dp, seq, cfg->loglik, result_model(result, DCP_ALT));

    struct dcp_model* null = result_model(result, DCP_NULL);
    model_set_loglik(null, imm_lprob_invalid());
    model_set_result(null, null->result);
    if (cfg->null) {
        hmm = imm_model_hmm(nmm_profile_get_model(profile, 1));
        dp = imm_model_dp(nmm_profile_get_model(profile, 1));
        model_scan(hmm, dp, seq, cfg->loglik, null);
    }

    imm_seq_destroy(seq);
}

static void model_scan(struct imm_hmm* hmm, struct imm_dp* dp, struct imm_seq const* seq, bool calc_loglik,
                       struct dcp_model* model)
{
    struct imm_dp_task* task = imm_dp_task_create(dp);

    imm_dp_task_setup(task, seq);
    struct imm_result const* r = imm_dp_viterbi(dp, task);
    imm_float                loglik = imm_lprob_invalid();
    if (calc_loglik && !imm_path_empty(imm_result_path(r)))
        loglik = imm_hmm_loglikelihood(hmm, seq, imm_result_path(r));

    model_set_loglik(model, loglik);
    model_set_result(model, r);

    imm_dp_task_destroy(task);

    struct imm_path const* path = imm_result_path(r);
    struct imm_step const* step = imm_path_first(path);

    char* data = NULL;
    step = imm_path_first(path);
    size_t i = 0;
    while (step) {
        char const* name = imm_state_get_name(imm_step_state(step));

        string_grow(&model->path, strlen(name) + 3);
        data = string_data(&model->path);

        while (*name != '\0')
            data[i++] = *(name++);

        data[i++] = ':';

        if (imm_step_seq_len(step) == 0)
            data[i++] = '0';
        else if (imm_step_seq_len(step) == 1)
            data[i++] = '1';
        else if (imm_step_seq_len(step) == 2)
            data[i++] = '2';
        else if (imm_step_seq_len(step) == 3)
            data[i++] = '3';
        else if (imm_step_seq_len(step) == 4)
            data[i++] = '4';
        else if (imm_step_seq_len(step) == 5)
            data[i++] = '5';
        else if (imm_step_seq_len(step) == 6)
            data[i++] = '6';
        else if (imm_step_seq_len(step) == 7)
            data[i++] = '7';
        else if (imm_step_seq_len(step) == 8)
            data[i++] = '8';
        else if (imm_step_seq_len(step) == 9)
            data[i++] = '9';

        data[i++] = ',';

        step = imm_path_next(path, step);
    }
    if (i > 0)
        --i;
    data = string_data(&model->path);
    data[i] = '\0';

    path = imm_result_path(r);
    step = imm_path_first(path);
    NMM_CODON_DECL(codon, nmm_base_abc_derived(imm_seq_get_abc(seq)));
    uint32_t offset = 0;
    i = 0;
    string_grow(&model->codons, 1);
    data = string_data(&model->codons);
    while (step) {
        struct imm_state const* state = imm_step_state(step);
        if (imm_state_type_id(state) == NMM_FRAME_STATE_TYPE_ID) {

            struct nmm_frame_state const* f = nmm_frame_state_derived(state);
            struct imm_seq                subseq = IMM_SUBSEQ(seq, offset, imm_step_seq_len(step));
            nmm_frame_state_decode(f, &subseq, &codon);
            string_grow(&model->codons, 3);
            data = string_data(&model->codons);
            data[i++] = codon.a;
            data[i++] = codon.b;
            data[i++] = codon.c;
        }
        offset += imm_step_seq_len(step);
        step = imm_path_next(path, step);
    }
    data[i++] = '\0';
}

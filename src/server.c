#include "deciphon/deciphon.h"
#include "imm/imm.h"
#include "nmm/nmm.h"
#include "profile.h"
#include "profile_ring.h"
#include "result.h"
#include "result_ring.h"
#include "results.h"
#include "results_pool.h"
#include "sequence.h"
#include "task.h"
#include <nmm/frame_state.h>
#include <stdatomic.h>

#ifdef _OPENMP
#include <omp.h>
#endif

struct dcp_server
{
    atomic_bool          finished;
    atomic_uint          nrunning_tasks;
    char const*          filepath;
    struct profile_ring  profiles;
    struct dcp_input*    input;
    struct results_pool* rpool;
};

struct model_scan_result
{
    imm_float                loglik;
    struct imm_result const* result;
};

static struct model_scan_result model_scan(struct imm_hmm* hmm, struct imm_dp* dp, struct imm_seq const* seq,
                                           bool calc_loglik, struct stream* stream);
static void                     profile_consumer(struct dcp_server* server, struct dcp_task* task);
static void                     profile_producer(struct dcp_server* server);
static void                     scan(struct dcp_server* server, struct dcp_profile const* profile, char const* sequence,
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
    server->rpool = results_pool_create(128);
    return server;
}

void dcp_server_destroy(struct dcp_server const* server)
{
    free((void*)server->filepath);
    dcp_input_destroy(server->input);
    results_pool_destroy(server->rpool);
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
#pragma omp single nowait
        profile_producer(server);

#pragma omp task
        profile_consumer(server, task);
    }
}

static void profile_consumer(struct dcp_server* server, struct dcp_task* task)
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
            uint16_t               rid = 0;

            struct dcp_results* results = NULL;
            while ((results = results_pool_get(server->rpool)) == NULL)
                ck_pr_stall();

            while (seq) {

                if (rid == results_limit(results)) {
                    task_add_results(task, results);

                    while ((results = results_pool_get(server->rpool)) == NULL)
                        ck_pr_stall();
                    rid = 0;
                }
                struct dcp_result* r = results_get(results, rid++);
                scan(server, prof, seq->sequence, r, seqid++, task_cfg(task));
                seq = task_next_sequence(task, seq);
            }

            task_add_results(task, results);
            dcp_profile_destroy(prof, true);
        }
    } while (prof || !server->finished);
    server->nrunning_tasks--;
}

static void profile_producer(struct dcp_server* server)
{
    dcp_input_reset(server->input);
    while (!dcp_input_end(server->input)) {
        struct dcp_profile const* prof = dcp_input_read(server->input);
        profile_ring_push(&server->profiles, prof);
    }
    server->finished = true;
}

static void scan(struct dcp_server* server, struct dcp_profile const* profile, char const* sequence,
                 struct dcp_result* result, uint32_t seqid, struct dcp_task_cfg const* cfg)
{
    struct nmm_profile const* p = dcp_profile_nmm_profile(profile);
    struct imm_abc const*     abc = nmm_profile_abc(p);
    struct imm_seq const*     seq = imm_seq_create(sequence, abc);

    result->profid = dcp_profile_id(profile);
    result->seqid = seqid;

    struct imm_hmm* hmm = imm_model_hmm(nmm_profile_get_model(p, 0));
    struct imm_dp*  dp = imm_model_dp(nmm_profile_get_model(p, 0));
    profile_setup(hmm, dp, cfg->multiple_hits, imm_seq_length(seq), cfg->hmmer3_compat);
    struct model_scan_result alt = model_scan(hmm, dp, seq, cfg->loglik, result->alt);

    result->alt_loglik = alt.loglik;
    result->alt_result = alt.result;
    result->null_loglik = imm_lprob_invalid();
    result->null_result = NULL;
    if (cfg->null) {
        hmm = imm_model_hmm(nmm_profile_get_model(p, 1));
        dp = imm_model_dp(nmm_profile_get_model(p, 1));
        struct model_scan_result null = model_scan(hmm, dp, seq, cfg->loglik, result->null);
        result->null_loglik = null.loglik;
        result->null_result = null.result;
    }

    imm_seq_destroy(seq);
}

static struct model_scan_result model_scan(struct imm_hmm* hmm, struct imm_dp* dp, struct imm_seq const* seq,
                                           bool calc_loglik, struct stream* stream)
{
    struct imm_dp_task* task = imm_dp_task_create(dp);

    imm_dp_task_setup(task, seq);
    struct imm_result const* result = imm_dp_viterbi(dp, task);
    imm_float                loglik = imm_lprob_invalid();
    if (calc_loglik && !imm_path_empty(imm_result_path(result)))
        loglik = imm_hmm_loglikelihood(hmm, seq, imm_result_path(result));

    imm_dp_task_destroy(task);

    struct imm_path const* path = imm_result_path(result);
    struct imm_step const* step = imm_path_first(path);
    size_t                 size = 1;
    while (step) {
        size += strlen(imm_state_get_name(imm_step_state(step))) + 3;
        step = imm_path_next(path, step);
    }
    if (size > 1)
        --size;

    stream->path = realloc(stream->path, sizeof(*stream->path) * size);
    step = imm_path_first(path);
    size_t i = 0;
    while (step) {
        char const* name = imm_state_get_name(imm_step_state(step));
        while (*name != '\0')
            stream->path[i++] = *(name++);

        stream->path[i++] = ':';

        if (imm_step_seq_len(step) == 0)
            stream->path[i++] = '0';
        else if (imm_step_seq_len(step) == 1)
            stream->path[i++] = '1';
        else if (imm_step_seq_len(step) == 2)
            stream->path[i++] = '2';
        else if (imm_step_seq_len(step) == 3)
            stream->path[i++] = '3';
        else if (imm_step_seq_len(step) == 4)
            stream->path[i++] = '4';
        else if (imm_step_seq_len(step) == 5)
            stream->path[i++] = '5';
        else if (imm_step_seq_len(step) == 6)
            stream->path[i++] = '6';
        else if (imm_step_seq_len(step) == 7)
            stream->path[i++] = '7';
        else if (imm_step_seq_len(step) == 8)
            stream->path[i++] = '8';
        else if (imm_step_seq_len(step) == 9)
            stream->path[i++] = '9';

        stream->path[i++] = ',';

        step = imm_path_next(path, step);
    }
    if (i > 0)
        --i;
    stream->path[i] = '\0';

    stream->codons = realloc(stream->codons, sizeof(*stream->codons) * size * 2);
    path = imm_result_path(result);
    step = imm_path_first(path);
    NMM_CODON_DECL(codon, nmm_base_abc_derived(imm_seq_get_abc(seq)));
    uint32_t offset = 0;
    i = 0;
    while (step) {
        struct imm_state const* state = imm_step_state(step);
        if (imm_state_type_id(state) == NMM_FRAME_STATE_TYPE_ID) {

            struct nmm_frame_state const* f = nmm_frame_state_derived(state);
            struct imm_seq                subseq = IMM_SUBSEQ(seq, offset, imm_step_seq_len(step));
            nmm_frame_state_decode(f, &subseq, &codon);
            stream->codons[i++] = codon.a;
            stream->codons[i++] = codon.b;
            stream->codons[i++] = codon.c;
        }
        offset += imm_step_seq_len(step);
        step = imm_path_next(path, step);
    }
    stream->codons[i++] = '\0';

    return (struct model_scan_result){loglik, result};
}

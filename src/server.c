#include "deciphon/deciphon.h"
#include "imm/imm.h"
#include "nmm/nmm.h"
#include "profile.h"
#include "profile_ring.h"
#include "result.h"
#include "result_ring.h"
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
    char const*              stream;
    char const*              codon_stream;
};

static struct model_scan_result model_scan(struct imm_hmm* hmm, struct imm_dp* dp, struct imm_seq const* seq,
                                           bool calc_loglik);
static void                     profile_consumer(struct dcp_server* server, struct dcp_task* task);
static void                     profile_producer(struct dcp_server* server);
static struct dcp_result*       scan(struct dcp_server* server, struct dcp_profile const* profile, char const* sequence,
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

static void profile_producer(struct dcp_server* server)
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

static struct dcp_result* scan(struct dcp_server* server, struct dcp_profile const* profile, char const* sequence,
                               uint32_t seqid, struct dcp_task_cfg const* cfg)
{
    struct nmm_profile const* p = dcp_profile_nmm_profile(profile);
    struct imm_abc const*     abc = nmm_profile_abc(p);
    struct imm_seq const*     seq = imm_seq_create(sequence, abc);

    struct dcp_result* r = malloc(sizeof(*r));
    r->profid = dcp_profile_id(profile);
    r->seqid = seqid;

    struct imm_hmm* hmm = imm_model_hmm(nmm_profile_get_model(p, 0));
    struct imm_dp*  dp = imm_model_dp(nmm_profile_get_model(p, 0));
    profile_setup(hmm, dp, cfg->multiple_hits, imm_seq_length(seq), cfg->hmmer3_compat);
    struct model_scan_result alt = model_scan(hmm, dp, seq, cfg->loglik);

    r->alt_loglik = alt.loglik;
    r->alt_result = alt.result;
    r->alt_stream = alt.stream;
    r->alt_codon_stream = alt.codon_stream;
    r->null_loglik = imm_lprob_invalid();
    r->null_result = NULL;
    r->null_stream = NULL;
    r->null_codon_stream = NULL;
    if (cfg->null) {
        hmm = imm_model_hmm(nmm_profile_get_model(p, 1));
        dp = imm_model_dp(nmm_profile_get_model(p, 1));
        struct model_scan_result null = model_scan(hmm, dp, seq, cfg->loglik);
        r->null_loglik = null.loglik;
        r->null_result = null.result;
        r->null_stream = null.stream;
        r->null_codon_stream = null.codon_stream;
    }
    list_init(&r->link);

    imm_seq_destroy(seq);

    return r;
}

static struct model_scan_result model_scan(struct imm_hmm* hmm, struct imm_dp* dp, struct imm_seq const* seq,
                                           bool calc_loglik)
{
    struct imm_dp_task* task = imm_dp_task_create(dp);

    imm_dp_task_setup(task, seq);
    struct imm_result const* result = imm_dp_viterbi(dp, task);
    imm_float                loglik = imm_lprob_invalid();
    if (calc_loglik && !imm_path_empty(imm_result_path(result)))
        loglik = imm_hmm_loglikelihood(hmm, seq, imm_result_path(result));

    imm_dp_task_destroy(task);
    /* imm_float nmm_frame_state_decode(struct nmm_frame_state const* state, struct imm_seq const* seq, */
    /*                              struct nmm_codon* codon) */

    struct imm_path const* path = imm_result_path(result);
    struct imm_step const* step = imm_path_first(path);
    size_t                 size = 1;
    while (step) {
        size += strlen(imm_state_get_name(imm_step_state(step))) + 3;
        step = imm_path_next(path, step);
    }
    if (size > 1)
        --size;

    char* stream = malloc(sizeof(*stream) * size);
    step = imm_path_first(path);
    size_t i = 0;
    while (step) {
        char const* name = imm_state_get_name(imm_step_state(step));
        while (*name != '\0')
            stream[i++] = *(name++);

        stream[i++] = ':';

        if (imm_step_seq_len(step) == 0)
            stream[i++] = '0';
        else if (imm_step_seq_len(step) == 1)
            stream[i++] = '1';
        else if (imm_step_seq_len(step) == 2)
            stream[i++] = '2';
        else if (imm_step_seq_len(step) == 3)
            stream[i++] = '3';
        else if (imm_step_seq_len(step) == 4)
            stream[i++] = '4';
        else if (imm_step_seq_len(step) == 5)
            stream[i++] = '5';
        else if (imm_step_seq_len(step) == 6)
            stream[i++] = '6';
        else if (imm_step_seq_len(step) == 7)
            stream[i++] = '7';
        else if (imm_step_seq_len(step) == 8)
            stream[i++] = '8';
        else if (imm_step_seq_len(step) == 9)
            stream[i++] = '9';

        stream[i++] = ',';

        step = imm_path_next(path, step);
    }
    if (i > 0)
        --i;
    stream[i] = '\0';

    /* TODO: fix this dumb malloc */
    char* codon_stream = malloc(sizeof(*codon_stream) * size * 2);
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
            codon_stream[i++] = codon.a;
            codon_stream[i++] = codon.b;
            codon_stream[i++] = codon.c;
        }
        offset += imm_step_seq_len(step);
        step = imm_path_next(path, step);
    }
    codon_stream[i++] = '\0';

    return (struct model_scan_result){loglik, result, stream, realloc(codon_stream, i)};
}

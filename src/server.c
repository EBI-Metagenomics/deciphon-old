#include "dcp/dcp.h"
#include "imm/imm.h"
#include "llist.h"
#include "nmm/nmm.h"
#include "profile.h"
#include "profile_ring.h"
#include "result.h"
#include "result_ring.h"
#include "results.h"
#include "sequence.h"
#include "task.h"
#include "util.h"
#include <nmm/frame_state.h>
#include <pthread.h>
#include <stdatomic.h>

#ifdef _OPENMP
#include <omp.h>
#endif

enum signal
{
    SIGNAL_NONE,
    SIGNAL_STOP,
};

enum status
{
    STATUS_CREATED,
    STATUS_STARTED,
    STATUS_STOPPED,
};

struct dcp_server
{
    pthread_t           loop_thread;
    char const*         filepath;
    struct profile_ring profiles;
    struct dcp_input*   input;
    struct llist_list   tasks;
    int                 producer_finished;
    int                 signal;
    int                 status;
};

struct model_scan_result
{
    imm_float                loglik;
    struct imm_result const* result;
};

static void  model_scan(struct imm_hmm* hmm, struct imm_dp* dp, struct imm_seq const* seq, bool calc_loglik,
                        struct dcp_model* model);
static void  profile_consumer(struct dcp_server* server, struct dcp_task* task);
static void  profile_producer(struct dcp_server* server);
static void  scan(struct dcp_server* server, struct nmm_profile const* profile, char const* sequence,
                  struct dcp_result* result, struct dcp_task_cfg const* cfg);
static void* server_loop(void* server);

void dcp_server_add(struct dcp_server* server, struct dcp_task* task)
{
#pragma omp critical
    llist_add(&server->tasks, &task->link);
}

struct dcp_server* dcp_server_create(char const* filepath)
{
    struct dcp_server* server = malloc(sizeof(*server));
    server->filepath = strdup(filepath);
    server->profiles = profile_ring_init();
    server->input = dcp_input_create(server->filepath);
    if (!server->input) {
        free((void*)server->filepath);
        free(server);
        return NULL;
    }
    llist_init_list(&server->tasks);
    server->producer_finished = 0;
    server->signal = SIGNAL_NONE;
    server->status = STATUS_CREATED;
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

int dcp_server_start(struct dcp_server* server)
{
    int rc = pthread_create(&server->loop_thread, NULL, server_loop, (void*)server);
    if (rc) {
        error("could not start loop thread: %d", rc);
        return 1;
    }
    return 0;
}

void dcp_server_stop(struct dcp_server* server)
{
    ck_pr_store_int(&server->signal, SIGNAL_STOP);

    while (ck_pr_load_int(&server->status) != STATUS_STOPPED)
        ck_pr_stall();
}

static void profile_consumer(struct dcp_server* server, struct dcp_task* task)
{
    printf("profile_consumer begin\n");
    fflush(stdout);
    struct dcp_profile const* prof = NULL;
    do {
        while (!(prof = profile_ring_pop(&server->profiles)) && !ck_pr_load_int(&server->producer_finished)) {
            ck_pr_stall();
        }

        if (prof) {
            struct sequence const* seq = task_first_seq(task);
            uint32_t               seqid = 0;

            struct dcp_results* results = NULL;
            while ((results = task_alloc_results(task)) == NULL) {
                /* printf("1"); */
                /* fflush(stdout); */
                ck_pr_stall();
            }
            printf("PASSOU: %p\n", (void*)seq);
            fflush(stdout);

            while (seq) {
                struct dcp_result* r = results_next(results);

                if (!r) {
                    task_push_results(task, results);
                    while ((results = task_alloc_results(task)) == NULL) {
                        /* printf("2"); */
                        /* fflush(stdout); */
                        ck_pr_stall();
                    }
                    continue;
                }

                result_set_profid(r, dcp_profile_id(prof));
                result_set_seqid(r, seqid);
                /* scan(server, dcp_profile_nmm_profile(prof), seq->sequence, r, task_cfg(task)); */
                seq = task_next_seq(task, seq);
            }

            if (dcp_results_size(results) > 0)
                task_push_results(task, results);

            dcp_profile_destroy(prof, true);
        }
    } while (prof || !ck_pr_load_int(&server->producer_finished));

    printf("profile_consumer end\n");
    fflush(stdout);
}

static void profile_producer(struct dcp_server* server)
{
    printf("profile_producer begin\n");
    ck_pr_store_int(&server->producer_finished, 0);
    dcp_input_reset(server->input);
    while (!dcp_input_end(server->input)) {
        struct dcp_profile const* prof = dcp_input_read(server->input);
        profile_ring_push(&server->profiles, prof);
    }
    ck_pr_store_int(&server->producer_finished, 1);
    printf("profile_producer end\n");
    fflush(stdout);
}

static void scan(struct dcp_server* server, struct nmm_profile const* profile, char const* sequence,
                 struct dcp_result* result, struct dcp_task_cfg const* cfg)
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

static void* server_loop(void* server_ptr)
{
    struct dcp_server* server = server_ptr;

    while (ck_pr_load_int(&server->signal) != SIGNAL_STOP) {

        struct dcp_task* task = NULL;
        /* printf("server_loop 1\n"); */
        /* fflush(stdout); */

        while (ck_pr_load_int(&server->signal) != SIGNAL_STOP && !task) {
            ck_pr_stall();

            /* printf("server_loop 2\n"); */
            /* fflush(stdout); */

#pragma omp critical
            {
                /* printf("server_loop 3\n"); */
                /* fflush(stdout); */
                struct llist_node* node = llist_pop(&server->tasks);
                if (node) {
                    task = CONTAINER_OF(node, struct dcp_task, link);
                }
            }
        }

        if (!task)
            continue;

#pragma omp        parallel default(none) shared(server, task)
        {
#pragma omp single nowait
            profile_producer(server);

#pragma omp task
            profile_consumer(server, task);
        }

        task_finish(task);
    }
    ck_pr_store_int(&server->status, STATUS_STOPPED);
    return NULL;
}

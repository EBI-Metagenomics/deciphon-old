#include "bus.h"
#include "dcp/dcp.h"
#include "dthread.h"
#include "fifo1.h"
#include "imm/imm.h"
#include "mpool.h"
#include "results.h"
#include "scan.h"
#include "seq.h"
#include "task.h"
#include "task_bin.h"
#include <errno.h>
#include <pthread.h>
#include <time.h>

#ifdef _OPENMP
#include <omp.h>
#endif

struct thread
{
    int       active;
    pthread_t id;
};

struct dcp_server
{
    struct thread     main_thread;
    struct bus        profile_bus;
    struct dcp_input* input;
    struct fifo1*     tasks;
    struct task_bin*  task_bin;
    int               stop_signal;
    struct mpool*     mpool;
};

static inline bool         active(struct thread const* thr) { return ck_pr_load_int(&thr->active); }
static struct dcp_results* alloc_results(struct dcp_server* server);
bool                       collect_task(struct dcp_task* task, void* arg);
void                       deinit_results(void* results);
void                       init_results(void* results);
static int                 input_processor(struct dcp_server* server);
static void*               main_thread(void* server_addr);
static inline bool         sigstop(struct dcp_server* server) { return ck_pr_load_int(&server->stop_signal); }
static int                 task_processor(struct dcp_server* server, struct dcp_task* task);
static int                 timedjoin(struct dcp_server* server);

void dcp_server_add_task(struct dcp_server* server, struct dcp_task* task) { fifo1_push(server->tasks, &task->node); }

struct dcp_server* dcp_server_create(char const* filepath)
{
    struct dcp_server* server = malloc(sizeof(*server));
    server->main_thread.active = 0;
    bus_init(&server->profile_bus);
    server->input = NULL;
    server->task_bin = NULL;
    server->tasks = fifo1_create();
    server->stop_signal = 0;

    if (!(server->input = dcp_input_create(filepath)))
        goto err;

    if (!(server->task_bin = task_bin_create(collect_task, server)))
        goto err;

    server->mpool = mpool_create(sizeof(struct dcp_results), 8, init_results);
    return server;

err:
    if (server->input)
        dcp_input_destroy(server->input);
    if (server->task_bin)
        task_bin_destroy(server->task_bin);
    free(server);
    return NULL;
}

int dcp_server_destroy(struct dcp_server* server)
{
    if (active(&server->main_thread)) {
        warn("destroying active server");
        if (timedjoin(server)) {
            error("failed to destroy server");
            return 1;
        }
    }

    BUG(!bus_end(&server->profile_bus));
    int err = 0;
    if (dcp_input_destroy(server->input)) {
        error("could not destroy input");
        err = 1;
    }
    if (task_bin_destroy(server->task_bin)) {
        error("could not destroy task_bin");
        err = 1;
    } else
        fifo1_destroy(server->tasks);
    mpool_destroy(server->mpool, deinit_results);
    free(server);
    return err;
}

void dcp_server_free_results(struct dcp_server* server, struct dcp_results* results)
{
    mpool_free(server->mpool, results);
}

void dcp_server_free_task(struct dcp_server* server, struct dcp_task* task) { task_bin_put(server->task_bin, task); }

int dcp_server_join(struct dcp_server* server)
{
    if (pthread_join(server->main_thread.id, NULL)) {
        error("failed to join main_thread");
        return 1;
    }
    if (task_bin_join(server->task_bin)) {
        error("failed to join task_bin");
        return 1;
    }
    return 0;
}

struct dcp_metadata const* dcp_server_metadata(struct dcp_server const* server, uint32_t profid)
{
    return dcp_input_metadata(server->input, profid);
}

uint32_t dcp_server_nprofiles(struct dcp_server const* server) { return dcp_input_nprofiles(server->input); }

int dcp_server_start(struct dcp_server* server)
{
    if (task_bin_start(server->task_bin))
        return 1;

    ck_pr_store_int(&server->main_thread.active, 1);
    if (pthread_create(&server->main_thread.id, NULL, main_thread, (void*)server)) {
        ck_pr_store_int(&server->main_thread.active, 0);
        error("could not spawn main_thread");
        return 1;
    }
    return 0;
}

void dcp_server_stop(struct dcp_server* server)
{
    ck_pr_store_int(&server->stop_signal, 1);
    task_bin_stop(server->task_bin);
}

static struct dcp_results* alloc_results(struct dcp_server* server)
{
    struct dcp_results* results = NULL;
    while (!(results = mpool_alloc(server->mpool)) && !sigstop(server))
        ck_pr_stall();

    if (results)
        results_rewind(results);

    return results;
}

bool collect_task(struct dcp_task* task, void* arg)
{
    if (!dcp_task_end(task))
        return false;

    dcp_task_destroy(task);
    return true;
}

void deinit_results(void* results) { results_deinit(results); }

void init_results(void* results) { results_init(results); }

static int input_processor(struct dcp_server* server)
{
    int err = 0;
    if (dcp_input_reset(server->input)) {
        err = 1;
        goto cleanup;
    }

    while (!dcp_input_end(server->input)) {

        struct dcp_profile const* prof = dcp_input_read(server->input);
        if (!prof) {
            err = 1;
            goto cleanup;
        }

        bool ok = false;
        while (!(ok = bus_send(&server->profile_bus, prof)) && !sigstop(server))
            ck_pr_stall();

        if (!ok) {
            dcp_profile_destroy(prof, true);
            err = 1;
            goto cleanup;
        }
    }

cleanup:
    bus_close_input(&server->profile_bus);
    return err;
}

static void* main_thread(void* server_addr)
{
    struct dcp_server* server = server_addr;

    while (!sigstop(server)) {

        struct fifo1_node* node = NULL;
        /* TODO: put to sleep if there is no task */
        while (!sigstop(server) && !(node = fifo1_pop(server->tasks)))
            ck_pr_stall();

        if (!node)
            continue;

        struct dcp_task* task = CONTAINER_OF(node, struct dcp_task, node);

        int errors = 0;
#pragma omp parallel default(none) shared(server, task) reduction(+ : errors)
        {
#pragma omp single
            bus_open_input(&server->profile_bus);

#pragma omp single nowait
            errors += input_processor(server);

            errors += task_processor(server, task);
        }
        if (errors)
            task_seterr(task);

        task_bin_collect(server->task_bin);
    }
    ck_pr_store_int(&server->main_thread.active, 0);
    pthread_exit(NULL);
    return NULL;
}

static int task_processor(struct dcp_server* server, struct dcp_task* task)
{
    int err = 0;
    while (!bus_end(&server->profile_bus) && !sigstop(server)) {

        struct dcp_profile const* prof = NULL;
        if (!(prof = bus_recv(&server->profile_bus))) {
            ck_pr_stall();
            continue;
        }

        struct dcp_results* results = alloc_results(server);
        if (!results) {
            BUG(!sigstop(server));
            dcp_profile_destroy(prof, true);
            continue;
        }

        struct iter_snode it = task_seqiter(task);
        struct seq const* seq = NULL;
        ITER_FOREACH(seq, &it, node)
        {
            struct dcp_result* r = results_next(results);
            if (!r) {
                task_add_results(task, results);

                if (!(results = alloc_results(server))) {
                    err++;
                    break;
                }
                r = results_next(results);
            }

            result_set_profid(r, dcp_profile_id(prof));
            result_set_seqid(r, seq_id(seq));
            scan(prof, seq, r, task_cfg(task));
        }

        if (results)
            task_add_results(task, results);

        dcp_profile_destroy(prof, true);
    }

    return err;
}

static int timedjoin(struct dcp_server* server)
{
    for (unsigned i = 0; i < 3; ++i) {
        dcp_server_stop(server);
        msleep(100 + i * 250);
        if (!active(&server->main_thread))
            return 0;
    }
    return 1;
}

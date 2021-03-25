#include "bus.h"
#include "dcp/dcp.h"
#include "imm/imm.h"
#include "mpool.h"
#include "results.h"
#include "scan.h"
#include "seq.h"
#include "task.h"
#include "task_queue.h"
#include <pthread.h>

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
    pthread_t         server_loop;
    struct bus        profile_bus;
    struct dcp_input* input;
    struct task_queue tasks;
    int               signal;
    int               status;
    struct mpool*     mpool;
};

static struct dcp_results* alloc_results(struct dcp_server* server);
static int                 input_processor(struct dcp_server* server);
static void*               server_loop(void* server_addr);
static inline bool         sigstop(struct dcp_server* server);
static int                 task_processor(struct dcp_server* server, struct dcp_task* task);

void dcp_server_add(struct dcp_server* server, struct dcp_task* task) { task_queue_push(&server->tasks, task); }

struct dcp_server* dcp_server_create(char const* filepath)
{
    struct dcp_server* server = malloc(sizeof(*server));
    bus_init(&server->profile_bus);
    if (!(server->input = dcp_input_create(filepath))) {
        free(server);
        return NULL;
    }
    task_queue_init(&server->tasks);
    server->signal = SIGNAL_NONE;
    server->status = STATUS_CREATED;

    static unsigned const pool_size = 8;
    /* static unsigned const pool_size = 4; */
    server->mpool = mpool_create(sizeof(struct dcp_results), pool_size);
    for (unsigned i = 0; i < pool_size; ++i)
        results_init(mpool_slot(server->mpool, i));

    return server;
}

void dcp_server_destroy(struct dcp_server* server)
{
    bus_deinit(&server->profile_bus);
    task_queue_deinit(&server->tasks);
    dcp_input_destroy(server->input);
    mpool_destroy(server->mpool);
    free(server);
}

void dcp_server_join(struct dcp_server* server)
{
    void* ret = NULL;
    BUG(pthread_join(server->server_loop, &ret));
}

struct dcp_metadata const* dcp_server_metadata(struct dcp_server const* server, uint32_t profid)
{
    return dcp_input_metadata(server->input, profid);
}

uint32_t dcp_server_nprofiles(struct dcp_server const* server) { return dcp_input_nprofiles(server->input); }

void dcp_server_recyle(struct dcp_server* server, struct dcp_results* results) { mpool_free(server->mpool, results); }

void dcp_server_start(struct dcp_server* server)
{
    BUG(pthread_create(&server->server_loop, NULL, server_loop, (void*)server));
}

void dcp_server_stop(struct dcp_server* server) { ck_pr_store_int(&server->signal, SIGNAL_STOP); }

static struct dcp_results* alloc_results(struct dcp_server* server)
{
    struct dcp_results* results = NULL;
    while (!(results = mpool_alloc(server->mpool)) && !sigstop(server))
        ck_pr_stall();

    if (results)
        results_rewind(results);

    return results;
}

static int input_processor(struct dcp_server* server)
{
    if (dcp_input_reset(server->input))
        return 1;

    int errno = 0;
    while (!dcp_input_end(server->input)) {

        struct dcp_profile const* prof = dcp_input_read(server->input);
        if (!prof) {
            errno = 1;
            break;
        }

        bool ok = false;
        while (!(ok = bus_send(&server->profile_bus, prof)) && !sigstop(server))
            ck_pr_stall();

        if (!ok) {
            dcp_profile_destroy(prof, true);
            errno = 1;
            break;
        }
    }

    bus_close_input(&server->profile_bus);
    return errno;
}

static void* server_loop(void* server_addr)
{
    struct dcp_server* server = server_addr;

    while (!sigstop(server)) {

        struct dcp_task* task = NULL;
        /* TODO: put to sleep if there is no task */
        while (!sigstop(server) && !(task = task_queue_pop(&server->tasks)))
            ck_pr_stall();

        if (!task)
            continue;

        int errors = 0;
#pragma omp parallel default(none) shared(server, task) reduction(+ : errors)
        {
#pragma omp single
            bus_open_input(&server->profile_bus);

#pragma omp single nowait
            errors += input_processor(server);

            errors += task_processor(server, task);
        }
        task_set_status(task, errors ? TASK_STATUS_STOPPED : TASK_STATUS_FINISHED);
    }
    ck_pr_store_int(&server->status, STATUS_STOPPED);
    return NULL;
}

static inline bool sigstop(struct dcp_server* server) { return ck_pr_load_int(&server->signal) == SIGNAL_STOP; }

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

        struct iter_snode it = task_seq_iter(task);
        struct seq const* seq = NULL;
        ITER_FOREACH(seq, &it, node)
        {
            printf("seq\n");
            fflush(stdout);
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

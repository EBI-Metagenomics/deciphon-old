#include "bus.h"
#include "dcp/dcp.h"
#include "imm/imm.h"
#include "mpool.h"
#include "results.h"
#include "scan.h"
#include "seq.h"
#include "task.h"
#include "task_bin.h"
#include "task_queue.h"
#include <errno.h>
#include <pthread.h>
#include <time.h>

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

struct gc
{
    pthread_t       thread;
    pthread_cond_t  cond;
    pthread_mutex_t mutex;
};

struct dcp_server
{
    pthread_t         server_loop;
    struct gc         gc;
    struct bus        profile_bus;
    struct dcp_input* input;
    struct task_queue tasks;
    struct task_bin   task_bin;
    int               signal;
    int               status;
    struct mpool*     mpool;
};

static struct dcp_results* alloc_results(struct dcp_server* server);
bool                       collect_task(struct dcp_task* task, void* arg);
static void*               garbage_collector(void* server_addr);
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
    task_bin_init(&server->task_bin);
    server->signal = SIGNAL_NONE;
    server->status = STATUS_CREATED;

    static unsigned const pool_power_size = 8;
    /* static unsigned const pool_power_size = 4; */
    server->mpool = mpool_create(sizeof(struct dcp_results), pool_power_size);
    for (unsigned i = 0; i < mpool_nslots(server->mpool); ++i) {
        results_init(mpool_slot(server->mpool, i));
    }

    return server;
}

void dcp_server_destroy(struct dcp_server* server)
{
    bus_deinit(&server->profile_bus);
    task_queue_deinit(&server->tasks);
    dcp_input_destroy(server->input);

    for (unsigned i = 0; i < mpool_nslots(server->mpool); ++i)
        results_deinit(mpool_slot(server->mpool, i));
    mpool_destroy(server->mpool);

    free(server);
}

void dcp_server_join(struct dcp_server* server)
{
    void* ret = NULL;
    BUG(pthread_join(server->server_loop, &ret));
    BUG(pthread_join(server->gc.thread, &ret));
}

struct dcp_metadata const* dcp_server_metadata(struct dcp_server const* server, uint32_t profid)
{
    return dcp_input_metadata(server->input, profid);
}

uint32_t dcp_server_nprofiles(struct dcp_server const* server) { return dcp_input_nprofiles(server->input); }

void dcp_server_free_results(struct dcp_server* server, struct dcp_results* results)
{
    mpool_free(server->mpool, results);
}

void dcp_server_free_task(struct dcp_server* server, struct dcp_task* task) { task_bin_put(&server->task_bin, task); }

void dcp_server_start(struct dcp_server* server)
{
    BUG(pthread_mutex_init(&server->gc.mutex, NULL));
    BUG(pthread_cond_init(&server->gc.cond, NULL));

    BUG(pthread_create(&server->gc.thread, NULL, garbage_collector, (void*)server));

    BUG(pthread_create(&server->server_loop, NULL, server_loop, (void*)server));
}

void error_pthread_cond_signal(const int signal_rv);

void error_pthread_cond_signal(const int signal_rv)
{
    fprintf(stderr, "Could not signal.\n");
    if (signal_rv == EINVAL) {
        fprintf(stderr, "The value cond does not refer to an initialised condition variable.\n");
    }
}

void dcp_server_stop(struct dcp_server* server)
{
    ck_pr_store_int(&server->signal, SIGNAL_STOP);
    const int signal_rv = pthread_cond_signal(&server->gc.cond);
    if (signal_rv) {
        error_pthread_cond_signal(signal_rv);
    }
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
    printf("collect_task: start\n");
    fflush(stdout);
    if (dcp_task_status(task) == TASK_STATUS_CREATED) {
        printf("collect_task: exit\n");
        fflush(stdout);
        return false;
    }

    printf("collect_task: destroy\n");
    fflush(stdout);

    dcp_task_destroy(task);
    return true;
}
void error_pthread_cond_timedwait(const int timed_wait_rv);

void error_pthread_cond_timedwait(const int timed_wait_rv)
{
    fprintf(stderr, "Conditional timed wait, failed.\n");
    switch (timed_wait_rv) {
    case ETIMEDOUT:
        fprintf(stderr, "The time specified by abstime to pthread_cond_timedwait() has passed.\n");
        break;
    case EINVAL:
        fprintf(stderr, "The value specified by abstime, cond or mutex is invalid.\n");
        break;
    case EPERM:
        fprintf(stderr, "The mutex was not owned by the current thread at the time of the call.\n");
        break;
    default:
        break;
    }
    fflush(stderr);
}

static void* garbage_collector(void* server_addr)
{
    struct dcp_server* server = server_addr;
    BUG(pthread_mutex_lock(&server->gc.mutex));
    while (!sigstop(server)) {
        struct timespec wait_time = {0, 0};
        BUG(clock_gettime(CLOCK_REALTIME, &wait_time));
        wait_time.tv_sec += 1;
        int rv = pthread_cond_timedwait(&server->gc.cond, &server->gc.mutex, &wait_time);

        if (rv) {
            error_pthread_cond_timedwait(rv);
        }
        fflush(stdout);
        task_bin_collect(&server->task_bin, collect_task, server);
    }
    task_bin_force_collect(&server->task_bin, collect_task, server);
    BUG(pthread_mutex_unlock(&server->gc.mutex));
    return NULL;
}

static int input_processor(struct dcp_server* server)
{
    if (dcp_input_reset(server->input))
        return 1;

    int err = 0;
    while (!dcp_input_end(server->input)) {

        struct dcp_profile const* prof = dcp_input_read(server->input);
        if (!prof) {
            err = 1;
            break;
        }

        bool ok = false;
        while (!(ok = bus_send(&server->profile_bus, prof)) && !sigstop(server))
            ck_pr_stall();

        if (!ok) {
            dcp_profile_destroy(prof, true);
            err = 1;
            break;
        }
    }

    bus_close_input(&server->profile_bus);
    return err;
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

        BUG(pthread_mutex_lock(&server->gc.mutex));
        BUG(pthread_cond_signal(&server->gc.cond));
        BUG(pthread_mutex_unlock(&server->gc.mutex));
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

#include "server.h"
#include "cco/cco.h"
#include "compiler.h"
#include "db.h"
#include "elapsed/elapsed.h"
#include "job.h"
#include "logger.h"
#include "protein_state.h"
#include "rc.h"
#include "sched.h"
#include "sched_db.h"
#include "sched_job.h"
#include "table.h"
#include "work.h"
#include "xfile.h"
#include <signal.h>

static struct server
{
    struct
    {
        volatile sig_atomic_t interrupt;
        struct sigaction action;
    } signal;
    unsigned num_threads;
    struct job job;
    struct prod prod;
} server = {0};

static void signal_interrupt(int signum) { server.signal.interrupt = 1; }

enum rc server_open(char const *filepath, unsigned num_threads)
{
    server.signal.action.sa_handler = &signal_interrupt;
    sigemptyset(&server.signal.action.sa_mask);
    sigaction(SIGINT, &server.signal.action, NULL);
    server.num_threads = num_threads;

    enum rc rc = RC_DONE;
    if ((rc = sched_setup(filepath))) return rc;
    return sched_open(filepath);
}

enum rc server_close(void) { return sched_close(); }

enum rc server_add_db(char const *name, char const *filepath, int64_t *id)
{
    if (!xfile_is_readable(filepath))
        return error(RC_IOERROR, "file is not readable");

    struct sched_db db = {0};
    enum rc rc = sched_db_setup(&db, name, filepath);
    if (rc) return rc;

    struct sched_db db2 = {0};
    rc = sched_db_get_by_xxh64(&db2, db.xxh64);
    if (rc == RC_NOTFOUND)
    {
        rc = sched_db_add(&db);
        *id = db.id;
        return rc;
    }
    *id = db2.id;
    return rc;
}

enum rc server_submit_job(struct job *job) { return sched_submit_job(job); }

enum rc server_job_state(int64_t job_id, enum job_state *state)
{
    return sched_job_state(job_id, state);
}

enum rc server_run(bool single_run)
{
    enum rc rc = RC_DONE;
    struct work work = {0};
    work_init(&work);

    info("Starting the server");
    while (!server.signal.interrupt)
    {
        if ((rc = work_next(&work)) == RC_NOTFOUND)
        {
            if (single_run) break;
            elapsed_sleep(500);
            continue;
        }
        if (rc != RC_NEXT) return rc;

        info("Found a new job");
        rc = work_run(&work, server.num_threads);
        if (rc) return rc;
        info("Finished a job");
    }

    info("Goodbye!");
    return RC_DONE;
}

enum rc server_next_prod(int64_t job_id, int64_t *prod_id)
{
    enum rc rc = sched_prod_next(job_id, prod_id);
    if (rc == RC_DONE) return rc;
    if (rc != RC_NEXT) return rc;
    if ((rc = sched_prod_get(&server.prod, *prod_id))) return rc;
    return RC_NEXT;
}

struct prod const *server_get_prod(void) { return &server.prod; }

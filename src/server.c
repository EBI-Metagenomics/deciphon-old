#include "server.h"
#include "cco/cco.h"
#include "compiler.h"
#include "db.h"
#include "dcp_sched/sched.h"
#include "elapsed/elapsed.h"
#include "job.h"
#include "logger.h"
#include "protein_state.h"
#include "rc.h"
#include "sched.h"
#include "table.h"
#include "work.h"
#include "xfile.h"
#include <signal.h>
#include <sqlite3.h>

static struct server
{
    struct
    {
        volatile sig_atomic_t interrupt;
        struct sigaction action;
    } signal;
    unsigned num_threads;
    struct job job;
    struct work work;
} server = {0};

static void signal_interrupt(int signum) { server.signal.interrupt = 1; }

enum rc server_open(char const *filepath, unsigned num_threads)
{
    server.signal.action.sa_handler = &signal_interrupt;
    sigemptyset(&server.signal.action.sa_mask);
    sigaction(SIGINT, &server.signal.action, NULL);
    server.num_threads = num_threads;

    if (sched_setup(filepath)) return error(RC_FAIL, "failed to setup sched");
    if (sched_open()) return error(RC_FAIL, "failed to open sched");
    return RC_DONE;
}

enum rc server_close(void) { return sched_close() ? RC_FAIL : RC_DONE; }

enum rc server_add_db(char const *filepath, int64_t *id)
{
    if (!xfile_is_readable(filepath))
        return error(RC_IOERROR, "file is not readable");

    if (sched_add_db(filepath, id)) return error(RC_FAIL, "failed to add db");
    return RC_DONE;
}

enum rc server_submit_job(struct job *job)
{
    struct sched_job j = {0};
    sched_job_init(&j, job->db_id, job->multi_hits, job->hmmer3_compat);
    if (sched_begin_job_submission(&j))
        return error(RC_FAIL, "failed to begin job submission");

    struct seq *seq = NULL;
    struct cco_iter iter = cco_queue_iter(&job->seqs);
    cco_iter_for_each_entry(seq, &iter, node)
    {
        sched_add_seq(&j, seq->name, seq->str.data);
    }

    if (sched_end_job_submission(&j))
        return error(RC_FAIL, "failed to end job submission");

    job->id = j.id;
    return RC_DONE;
}

enum rc server_job_state(int64_t job_id, enum sched_job_state *state)
{
    if (sched_job_state(job_id, state))
        return error(RC_FAIL, "failed to get job state");
    return RC_DONE;
}

enum rc server_run(bool single_run)
{
    enum rc rc = RC_DONE;

    info("Starting the server");
    while (!server.signal.interrupt)
    {
        if ((rc = work_next(&server.work)) == RC_NOTFOUND)
        {
            if (single_run) break;
            elapsed_sleep(500);
            continue;
        }
        if (rc != RC_NEXT) return rc;

        info("Found a new job");
        rc = work_run(&server.work, server.num_threads);
        if (rc) return rc;
        info("Finished a job");
    }

    info("Goodbye!");
    return RC_DONE;
}

#if 0
enum rc server_next_prod(int64_t job_id, int64_t *prod_id)
{
    enum rc rc = sched_prod_next(job_id, prod_id);
    if (rc == RC_DONE) return rc;
    if (rc != RC_NEXT) return rc;
    if ((rc = sched_prod_get(&server.prod, *prod_id))) return rc;
    return RC_NEXT;
}
#endif

/* struct prod const *server_get_prod(void) { return &server.prod; } */

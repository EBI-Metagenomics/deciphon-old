#include "server.h"
#include "rest.h"
#include "cco/cco.h"
#include "common/compiler.h"
#include "common/logger.h"
#include "common/rc.h"
#include "common/safe.h"
#include "common/xfile.h"
#include "common/xmath.h"
#include "db.h"
#include "elapsed/elapsed.h"
#include "job.h"
#include "protein_state.h"
#include "sched.h"
#include "table.h"
#include "work.h"
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
    struct work work;
} server = {0};

static void signal_interrupt(int signum) { server.signal.interrupt = 1; }

enum rc server_open(char const *filepath, unsigned num_threads)
{
    server.signal.action.sa_handler = &signal_interrupt;
    sigemptyset(&server.signal.action.sa_mask);
    sigaction(SIGINT, &server.signal.action, NULL);
    server.num_threads = num_threads;

    if (sched_setup(filepath)) return error(RC_EFAIL, "failed to setup sched");
    if (sched_open()) return error(RC_EFAIL, "failed to open sched");
    server.work.lrt_threshold = 100.0f;
    return RC_DONE;
}

enum rc server_close(void) { return sched_close() ? RC_EFAIL : RC_DONE; }

enum rc server_add_db(char const *filepath, int64_t *id)
{
    if (!xfile_is_readable(filepath))
        return error(RC_EIO, "file is not readable");

    if (sched_add_db(filepath, id)) return error(RC_EFAIL, "failed to add db");
    return RC_DONE;
}

enum rc server_submit_job(struct job *job)
{
    struct sched_job j = {0};
    sched_job_init(&j, job->db_id, job->multi_hits, job->hmmer3_compat);
    if (sched_begin_job_submission(&j))
        return error(RC_EFAIL, "failed to begin job submission");

    struct seq *seq = NULL;
    struct cco_iter iter = cco_queue_iter(&job->seqs);
    cco_iter_for_each_entry(seq, &iter, node)
    {
        sched_add_seq(&j, seq->name, seq->str.data);
    }

    if (sched_end_job_submission(&j))
        return error(RC_EFAIL, "failed to end job submission");

    job->id = j.id;
    return RC_DONE;
}

enum rc server_job_state(int64_t job_id, enum sched_job_state *state)
{
    if (sched_job_state(job_id, state))
        return error(RC_EFAIL, "failed to get job state");
    return RC_DONE;
}



enum rc server_run(bool single_run)
{
    enum rc rc = rest_job_state(1);
    return rc;
    return 0;

#if 0
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
#endif
}

void server_set_lrt_threshold(imm_float lrt)
{
    server.work.lrt_threshold = lrt;
}

enum rc server_get_sched_job(struct sched_job *job)
{
    if (sched_get_job(job)) return error(RC_EFAIL, "failed to get job");
    return RC_DONE;
}

enum rc server_next_sched_prod(struct sched_job const *job,
                               struct sched_prod *prod)
{
    prod->job_id = job->id;
    enum rc rc = sched_prod_next(prod);
    if (rc == RC_DONE) return RC_DONE;
    if (rc != RC_NEXT) return error(RC_EFAIL, "failed to get prod");
    return RC_NEXT;
}

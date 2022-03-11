#include "deciphon/server/server.h"
#include "deciphon/compiler.h"
#include "deciphon/logger.h"
#include "deciphon/rc.h"
#include "elapsed/elapsed.h"
#include "job.h"
// #include "protein_state.h"
#include "deciphon/server/rest.h"
#include "sched.h"
// #include "table.h"
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

// enum rc server_open(char const *filepath, unsigned num_threads)
// {
//     server.signal.action.sa_handler = &signal_interrupt;
//     sigemptyset(&server.signal.action.sa_mask);
//     sigaction(SIGINT, &server.signal.action, NULL);
//     server.num_threads = num_threads;
//
//     // if (sched_setup(filepath)) return error(DECIPHON_EFAIL, "failed to
//     setup sched");
//     // if (sched_open()) return error(DECIPHON_EFAIL, "failed to open
//     sched"); server.work.lrt_threshold = 100.0f; return DECIPHON_OK;
// }

// enum rc server_close(void) { return sched_close() ? DECIPHON_EFAIL :
// DECIPHON_OK; }

// enum rc server_add_db(char const *filepath, int64_t *id)
// {
//     if (!xfile_is_readable(filepath))
//         return error(RC_EIO, "file is not readable");
//
//     if (sched_add_db(filepath, id)) return error(DECIPHON_EFAIL, "failed to
//     add db"); return DECIPHON_OK;
// }

// enum rc server_submit_job(struct job *job)
// {
//     struct sched_job j = {0};
//     sched_job_init(&j, job->db_id, job->multi_hits, job->hmmer3_compat);
//     if (sched_begin_job_submission(&j))
//         return error(DECIPHON_EFAIL, "failed to begin job submission");
//
//     struct seq *seq = NULL;
//     struct cco_iter iter = cco_queue_iter(&job->seqs);
//     cco_iter_for_each_entry(seq, &iter, node)
//     {
//         sched_add_seq(&j, seq->name, seq->str.data);
//     }
//
//     if (sched_end_job_submission(&j))
//         return error(DECIPHON_EFAIL, "failed to end job submission");
//
//     job->id = j.id;
//     return DECIPHON_OK;
// }

// enum rc server_job_state(int64_t job_id, enum sched_job_state *state)
// {
//     if (sched_job_state(job_id, state))
//         return error(DECIPHON_EFAIL, "failed to get job state");
//     return DECIPHON_OK;
// }

enum rc server_run(bool single_run, unsigned num_threads, char const *url)
{
    server.signal.action.sa_handler = &signal_interrupt;
    sigemptyset(&server.signal.action.sa_mask);
    sigaction(SIGINT, &server.signal.action, NULL);
    server.num_threads = num_threads;
    server.work.lrt_threshold = 100.0f;

    enum rc rc = rest_open(url);
    if (rc) return rc;

    info("Starting the server (%d threads)", num_threads);
    while (!server.signal.interrupt)
    {
        if ((rc = work_next(&server.work)) == RC_END)
        {
            if (single_run) break;
            elapsed_sleep(500);
            continue;
        }
        if (rc == RC_END)
        {
            rc = RC_OK;
            goto cleanup;
        }
        if (rc != RC_OK) goto cleanup;

        info("Found a new job");
        rc = work_run(&server.work, server.num_threads);
        if (rc) goto cleanup;
        info("Finished a job");
    }

    if (rc == RC_END) rc = RC_OK;

    info("Goodbye!");
cleanup:
    rest_close();
    return rc;
}

void server_set_lrt_threshold(imm_float lrt)
{
    server.work.lrt_threshold = lrt;
}

// enum rc server_get_sched_job(struct sched_job *job)
// {
//     if (sched_get_job(job)) return error(DECIPHON_EFAIL, "failed to get
//     job"); return DECIPHON_OK;
// }

// enum rc server_next_sched_prod(struct sched_job const *job,
//                                struct sched_prod *prod)
// {
//     prod->job_id = job->id;
//     enum rc rc = sched_prod_next(prod);
//     if (rc == DECIPHON_OK) return DECIPHON_OK;
//     if (rc != RC_NEXT) return error(DECIPHON_EFAIL, "failed to get prod");
//     return RC_NEXT;
// }

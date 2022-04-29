#include "deciphon/server/server.h"
#include "deciphon/compiler.h"
#include "deciphon/info.h"
#include "deciphon/logger.h"
#include "deciphon/rc.h"
#include "deciphon/sched/api.h"
#include "elapsed/elapsed.h"
#include "job.h"
#include <signal.h>

static struct server
{
    struct server_cfg cfg;

    struct
    {
        volatile sig_atomic_t interrupt;
        struct sigaction action;
    } signal;

    unsigned initialized;
    struct job job;
} server = {0};

static void signal_handler(int signum)
{
    if (signum == SIGINT)
    {
        info("Terminating it...");
        server.signal.interrupt = 1;
        return;
    }
    efail("unknown signum");
}

enum rc server_init(char const *sched_api_url, struct server_cfg cfg)
{
    if (server.initialized++) return RC_OK;

    server.cfg = cfg;

    enum rc rc = api_init(sched_api_url, cfg.api_key);
    if (rc) return rc;

    if (cfg.single_run) server.signal.interrupt = 1;

    server.signal.action.sa_handler = &signal_handler;
    sigemptyset(&server.signal.action.sa_mask);
    if (sigaction(SIGINT, &server.signal.action, NULL))
    {
        rc = efail("failed to listen to SIGINT");
        goto cleanup;
    }

    return rc;

cleanup:
    api_cleanup();
    return rc;
}

enum rc server_run(void)
{
    enum rc rc = RC_OK;
    job_init(&server.job, server.cfg.num_threads);

    info("Starting the server (%d threads)", server.cfg.num_threads);

    do
    {
        server.job.sched.id = 0;
        rc = job_next(&server.job);
        if (rc == RC_OK)
        {
            info("Running job[%ld]", server.job.sched.id);
            rc = job_run(&server.job);
            if (rc)
            {
                info("Failed job[%ld]", server.job.sched.id);
                continue;
            }
            info("Finished job[%ld]", server.job.sched.id);
        }
        else if (rc == RC_END)
        {
            rc = RC_OK;
            elapsed_sleep(1000 / server.cfg.polling_rate);
        }

        if (rc && !server.signal.interrupt)
        {
            info("Backing off for 5s due to error");
            elapsed_sleep(5 * 1000);
        }

    } while (!server.signal.interrupt);

    info("Goodbye!");
    return rc;
}

void server_cleanup(void)
{
    if (!server.initialized) return;
    if (--server.initialized) return;

    api_cleanup();
}

#include "deciphon/server/server.h"
#include "deciphon/compiler.h"
#include "deciphon/info.h"
#include "deciphon/logger.h"
#include "deciphon/rc.h"
#include "deciphon/server/sched_api.h"
#include "elapsed/elapsed.h"
#include "work.h"
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
    struct work work;
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

    enum rc rc = sched_api_init(sched_api_url);
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
    sched_api_cleanup();
    return rc;
}

enum rc server_run(void)
{
    enum rc rc = RC_OK;

    info("Starting the server (%d threads)", server.cfg.num_threads);

    do
    {
        rc = work_next(&server.work);
        if (rc == RC_OK)
        {
            info("Found new job[%ld]", server.work.job.id);
            rc = work_prepare(&server.work, server.cfg.num_threads);
            if (rc) break;

            info("Running job[%ld]", server.work.job.id);
            rc = work_run(&server.work);
            if (rc) break;

            info("Finished job[%ld]", server.work.job.id);
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

    sched_api_cleanup();
}

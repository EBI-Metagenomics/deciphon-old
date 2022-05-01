#include "cli.h"
#include "deciphon/dotenv.h"
#include "deciphon/logger.h"
#include "deciphon/server/server.h"
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

void skeleton_daemon(void);

static void syslog_print(char const *msg, void *arg)
{
    printf("%s\n", msg);
    syslog(LOG_ERR, "%s", msg);
}

static void print_log_put(char const *msg, void *arg)
{
    (void)msg;
    (void)arg;
    // __log_put(LOG_ERROR, msg);
}

static void flush_nop(void *arg) {}

static char url_stem[2048] = {0};

enum rc cli_server(void)
{
    enum rc rc = RC_OK;

    if (dotenv_load(".env", true))
        return efail("failed to read `.env` file from the working directory");

    char const *host = getenv("API_HOST");
    if (!host) return efail("failed to get `API_HOST`");

    char const *port = getenv("API_PORT");
    if (!port) return efail("failed to get `API_PORT`");

    char const *prefix = getenv("API_PREFIX");
    if (!prefix) return efail("failed to get `API_PREFIX`");

    char const *key = getenv("API_KEY");
    if (!key) return efail("failed to get `API_KEY`");

    char const *num_threads = getenv("NUM_THREADS");
    if (!num_threads) return efail("failed to get `NUM_THREADS`");

    strcpy(url_stem, "http://");
    strcpy(url_stem + strlen(url_stem), host);
    strcpy(url_stem + strlen(url_stem), ":");
    strcpy(url_stem + strlen(url_stem), port);
    strcpy(url_stem + strlen(url_stem), prefix);

    // char const *url_stem = arguments.args[0];
    // char const *num_threads = arguments.args[1];

    /* skeleton_daemon(); */
    // logger_setup(print_log_put, NULL);
    // log_setup(LOG_ERROR, syslog_print, flush_nop, NULL);

    // rc = server_open(schedfile, 1);
    // if (rc) goto cleanup;
    //
    // int64_t db_id = 0;
    // rc = server_add_db(dcpfile, &db_id);
    // if (rc) goto cleanup;
    struct server_cfg cfg = SERVER_CFG_INIT;
    cfg.num_threads = (unsigned)atoi(num_threads);
    strcpy(cfg.api_key, key);

    rc = server_init(url_stem, cfg);
    if (rc) goto cleanup;

    rc = server_run();
    server_cleanup();
    if (rc) goto cleanup;

    // rc = server_close();
    // log_flush();
    // closelog();
    return rc;

cleanup:
    // server_close();
    // log_flush();
    // closelog();
    return rc;
}

void skeleton_daemon(void)
{
    /* Fork off the parent process */
    pid_t pid = fork();

    /* An error occurred */
    if (pid < 0) exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0) exit(EXIT_SUCCESS);

    /* On success: The child process becomes session leader */
    if (setsid() < 0) exit(EXIT_FAILURE);

    /* Catch, ignore and handle signals */
    /*TODO: Implement a working signal handler */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    /* Fork off for the second time*/
    pid = fork();

    /* An error occurred */
    if (pid < 0) exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0) exit(EXIT_SUCCESS);

    /* Set new file permissions */
    umask(0);

    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    chdir("/");

    /* Close all open file descriptors */
    for (long x = sysconf(_SC_OPEN_MAX); x >= 0; x--)
    {
        close((int)x);
    }
    /* Open the log file */
    openlog("deciphond", LOG_PID, LOG_DAEMON);
}

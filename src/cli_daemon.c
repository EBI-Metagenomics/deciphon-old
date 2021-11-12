#include "cli.h"
#include "dcp/cli.h"
#include "dcp/dcp.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

struct arguments
{
    char *args[1];
    int quiet;
} arguments;

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *args = state->input;

    switch (key)
    {
    case 'q':
        args->quiet = 1;
        break;

    case ARGP_KEY_ARG:
        if (state->arg_num >= 1) argp_usage(state);
        args->args[state->arg_num] = arg;
        break;

    case ARGP_KEY_END:
        if (state->arg_num < 1) argp_usage(state);
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static char doc[] = "Run daemon -- deciphond deciphon.sqlite3";
static char args_doc[] = "DBFILE";
static struct argp_option options[] = {
    {"quiet", 'q', 0, 0, "Disable output", 0}, {0}};
static struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, 0};

void skeleton_daemon(void);


enum dcp_rc dcp_cli_daemon(int argc, char **argv)
{
    if (argp_parse(&argp, argc, argv, 0, 0, &arguments)) return DCP_ILLEGALARG;

    enum dcp_rc rc = DCP_DONE;
    char const *dbfile = arguments.args[0];

    skeleton_daemon();
    syslog(LOG_NOTICE, "First daemon started.");
    sleep(1);

    rc = dcp_srv_open(dbfile);
    if (rc) goto cleanup;
    syslog(LOG_NOTICE, "dcp_srv_open");

    int64_t db_id = 0;
    rc = dcp_srv_add_db("pro_example1",
                        "/Users/horta/tmp/pro_example1.dcp", &db_id);
    syslog(LOG_NOTICE, "dcp_srv_add_db: %d", rc);
    if (rc) goto cleanup;

    rc = dcp_srv_run(false);
    syslog(LOG_NOTICE, "dcp_srv_run: %d", rc);
    if (rc) goto cleanup;

    rc = dcp_srv_close();
    syslog(LOG_NOTICE, "dcp_srv_close: %d", rc);
    closelog();
    return rc;

cleanup:
    dcp_srv_close();
    closelog();
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

#include "cli.h"
#include "log/log.h"
#include "logger.h"
#include "server.h"
#include "xfile.h"
#include <argp.h>
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

struct arguments
{
    char *args[2];
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
        if (state->arg_num >= 2) argp_usage(state);
        args->args[state->arg_num] = arg;
        break;

    case ARGP_KEY_END:
        if (state->arg_num < 2) argp_usage(state);
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static char doc[] = "Run daemon -- deciphond deciphon.sched pfam.dcp";
static char args_doc[] = "SCHEDFILE DCPFILE";
static struct argp_option options[] = {
    {"quiet", 'q', 0, 0, "Disable output", 0}, {0}};
static struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, 0};

void skeleton_daemon(void);

static void syslog_print(char const *msg, void *arg)
{
    printf("%s\n", msg);
    syslog(LOG_ERR, "%s", msg);
}

static void print_log_put(char const *msg, void *arg)
{
    __log_put(LOG_ERROR, msg);
}

static void flush_nop(void *arg) {}

enum rc cli_server(int argc, char **argv)
{
    if (argp_parse(&argp, argc, argv, 0, 0, &arguments)) return RC_ILLEGALARG;

    enum rc rc = RC_DONE;
    char const *schedfile = arguments.args[0];
    char const *dcpfile = arguments.args[1];

    /* skeleton_daemon(); */
    logger_setup(print_log_put, NULL);
    log_setup(LOG_ERROR, syslog_print, flush_nop, NULL);

    rc = server_open(schedfile, 1);
    if (rc) goto cleanup;

    int64_t db_id = 0;
    rc = server_add_db(dcpfile, &db_id);
    if (rc) goto cleanup;

    rc = server_run(false);
    if (rc) goto cleanup;

    rc = server_close();
    log_flush();
    closelog();
    return rc;

cleanup:
    server_close();
    log_flush();
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

#include "posix.h"

#include "core/daemonize.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <unistd.h>

static void sanitize_fp(FILE *fp, char const *mode);
static void close_nonstd_fds(void);
static void become_session_leader(void);

void daemonize(bool sanitize_stdin, bool sanitize_stdout, bool sanitize_stderr,
               bool close_nstd)
{
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    fflush(stdout);
    fflush(stderr);

    switch (fork())
    {
    case 0:
        if (sanitize_stdin) sanitize_fp(stdin, "r");
        if (sanitize_stdout) sanitize_fp(stdout, "a");
        if (sanitize_stderr) sanitize_fp(stderr, "a");
        if (close_nstd) close_nonstd_fds();
        become_session_leader();
        break;
    case -1:
        perror("In fork():");
        exit(EXIT_FAILURE);
        break;
    default:
        exit(EXIT_SUCCESS);
    }
}

static void sanitize_fp(FILE *fp, char const *mode)
{
    if (!freopen("/dev/null", mode, fp)) exit(EXIT_FAILURE);
}

static void become_session_leader(void)
{
    if (setsid() == -1) exit(EXIT_FAILURE);
}

/* Source: OpenSIPS */
static void close_nonstd_fds(void)
{
    /* 32 is the maximum number of inherited open file descriptors */
    for (int r = 3; r < 32; r++)
        close(r);
}

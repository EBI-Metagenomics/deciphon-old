#include "core/posix.h"

#include "core/logy.h"
#include <stdlib.h>
#include <unistd.h>

static void close_all_file_descriptors(void);

void daemonize(void)
{
    switch (fork())
    {
    case 0:
        break;
    case -1:
        fatal("fork failed");
    default:
        exit(0);
    }
    if (setsid() == -1) fatal("setsid failed");
    close_all_file_descriptors();
}

static void close_all_file_descriptors(void)
{
    /* Close all open file descriptors */
    for (int x = sysconf(_SC_OPEN_MAX); x >= 0; x--)
    {
        close(x);
    }
}

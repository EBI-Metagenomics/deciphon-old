#include "core/pidfile.h"
#include "core/logy.h"
#include <stdio.h>
#include <unistd.h>

void pidfile_save(char const *filepath)
{
    FILE *fp = fopen(filepath, "wb");
    if (!fp) fatal("failed to open pid file");

    int pid = getpid();
    if (fprintf(fp, "%d\n", pid) < 0) fatal("failed to write pid file");

    if (fclose(fp)) fatal("failed to close pid file");
}

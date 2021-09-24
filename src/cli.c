#include "cli.h"
#include "dcp/log.h"
#include "imm/imm.h"
#include "log/log.h"
#include <stdio.h>

static void wrap_log_put(char const *msg, void *arg)
{
    __log_put(LOG_ERROR, msg);
}

static void wrap_fflush(void *arg)
{
    FILE *fd = arg;
    fflush(fd);
}

static void wrap_fprintf(char const *msg, void *arg)
{
    fprintf(stderr, "%s\n", msg);
}

void cli_log_setup(void)
{
    imm_log_setup(wrap_log_put, NULL);
    dcp_log_setup(wrap_log_put, NULL);
    log_setup(LOG_ERROR, wrap_fprintf, wrap_fflush, NULL);
}

void cli_log_flush(void) { log_flush(); }

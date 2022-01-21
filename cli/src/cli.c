#include "cli.h"
#include "imm/imm.h"
#include "log/log.h"
#include "common/logger.h"
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

void cli_logger_setup(void)
{
    imm_log_setup(wrap_log_put, NULL);
    logger_setup(wrap_log_put, NULL);
    log_setup(LOG_ERROR, wrap_fprintf, wrap_fflush, NULL);
}

void cli_logger_flush(void) { log_flush(); }

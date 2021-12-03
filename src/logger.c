#include "logger.h"
#include <stdio.h>

static void default_print(char const *msg, void *arg)
{
    fprintf(stderr, "%s\n", msg);
}

static dcp_log_print_t *__log_print = default_print;
static void *__log_arg = NULL;

void dcp_log_setup(dcp_log_print_t *print, void *arg)
{
    __log_print = print;
    __log_arg = arg;
}

static void log_print(char const *msg) { __log_print(msg, __log_arg); }

enum dcp_rc __logger_error(enum dcp_rc rc, char const *msg)
{
    log_print(msg);
    return rc;
}

enum dcp_rc __logger_warn(enum dcp_rc rc, char const *msg)
{
    log_print(msg);
    return rc;
}

void __logger_info(char const *msg) { log_print(msg); }

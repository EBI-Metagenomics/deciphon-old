#include "deciphon/util/logger.h"
#include "deciphon/rc.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static void default_print(char const *msg, void *arg)
{
    (void)arg;
    fprintf(stderr, "%s\n", msg);
}

static logger_print_t *__log_print = default_print;
static void *__log_arg = NULL;

void logger_setup(logger_print_t *print, void *arg)
{
    __log_print = print;
    __log_arg = arg;
}

static void log_print(char const *msg) { __log_print(msg, __log_arg); }

enum rc __logger_error(enum rc rc, char const *msg)
{
    log_print(msg);
    return rc;
}

enum rc __logger_warn(enum rc rc, char const *msg)
{
    log_print(msg);
    return rc;
}

// void __logger_info(char const *msg) { log_print(msg); }
void info(char const *fmt, ...)
{
    time_t timer = {0};
    char stamp[26];
    struct tm *tm_info = 0;
    timer = time(NULL);
    tm_info = localtime(&timer);
    strftime(stamp, 26, "%H:%M:%S", tm_info);
    // strftime(stamp, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    fprintf(stdout, "[%s] ", stamp);

    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);

    fprintf(stdout, "\n");
    fflush(stdout);
}

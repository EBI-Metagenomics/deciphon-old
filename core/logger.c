#include "deciphon/logger.h"
#include "deciphon/limits.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static void default_print(char const *ctx, char const *msg, void *arg)
{
    (void)arg;
    fputs(ctx, stderr);
    fputs(": ", stderr);
    fputs(msg, stderr);
    fputc('\n', stderr);
}

static void default_print_static(char const *ctx, char const *msg, void *arg)
{
    (void)arg;
    fputs(ctx, stderr);
    fputs(": ", stderr);
    fputs(msg, stderr);
    fputc('\n', stderr);
}

static struct
{
    logger_print_func_t print;
    logger_print_static_func_t print_static;
    void *arg;
} local = {default_print, default_print_static, 0};

void logger_setup(logger_print_func_t print,
                  logger_print_static_func_t print_static, void *arg)
{
    local.print = print;
    local.print_static = print_static;
    local.arg = arg;
}

void info(char const *fmt, ...)
{
    time_t timer = {0};
    char stamp[26];
    struct tm *tm_info = 0;
    timer = time(NULL);
    tm_info = localtime(&timer);
    strftime(stamp, 26, "%H:%M:%S", tm_info);
    fprintf(stdout, "[%s] ", stamp);

    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);

    fprintf(stdout, "\n");
    fflush(stdout);
}

enum rc __logger_error(enum rc rc, char const *ctx, char const *msg)
{
    local.print(msg, ctx, local.arg);
    return rc;
}

enum rc __logger_error_static(enum rc rc, char const *ctx, char const *msg)
{
    local.print_static(ctx, msg, local.arg);
    return rc;
}

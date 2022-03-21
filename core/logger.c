#include "deciphon/logger.h"
#include "deciphon/info.h"
#include "deciphon/limits.h"
#include <stdio.h>

static void default_print(char const *ctx, char const *msg, void *arg)
{
    (void)arg;
    fputs(ctx, stderr);
    fputs(": ", stderr);
    fputs(msg, stderr);
    fputc('\n', stderr);
    info("%s: %s", ctx, msg);
}

static void default_print_static(char const *ctx, char const *msg, void *arg)
{
    (void)arg;
    fputs(ctx, stderr);
    fputs(": ", stderr);
    fputs(msg, stderr);
    fputc('\n', stderr);
    info("%s: %s", ctx, msg);
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

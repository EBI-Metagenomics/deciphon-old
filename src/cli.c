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

static bool append_ext(char *str, size_t len, size_t max_size, char const *ext)
{
    char *j = &str[len];
    size_t n = strlen(ext);
    if (n + 1 + (size_t)(j - str) > max_size) return false;
    *(j++) = *(ext++);
    *(j++) = *(ext++);
    *(j++) = *(ext++);
    *(j++) = *(ext++);
    *j = *ext;
    return true;
}

static bool change_ext(char *str, size_t pos, size_t max_size, char const *ext)
{
    char *j = &str[pos];
    while (j > str && *j != '.')
        --j;
    if (j == str) return false;
    return append_ext(str, (size_t)(j - str), max_size, ext);
}

bool cli_change_or_add_ext(char *str, size_t max_size, char const *ext)
{
    size_t len = strlen(str);
    if (!change_ext(str, len, max_size, ext))
        return append_ext(str, len, max_size, ext);
    return true;
}

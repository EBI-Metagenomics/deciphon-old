#include "core/logging.h"
#include "core/spinlock.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static char stamp[] = "[24:00:00]";
static spinlock_t lock = SPINLOCK_INIT;

#define USER_PIPE 0
#define SYS_PIPE 1

static FILE *streams[2] = {0};
static enum logging_level levels[2] = {LOGGING_NOTSET};

static FILE *open_file(char const *file, char const *default_file);
static void pipe_into(FILE *restrict stream, char const *ctx, char const *fmt,
                      va_list args);
static void initialize_if_not_yet(void);

void logging_setup(char const *user_file, enum logging_level user_level,
                   char const *sys_file, enum logging_level sys_level)
{
    streams[USER_PIPE] = open_file(user_file, "&1");
    streams[SYS_PIPE] = open_file(sys_file, "&2");

    levels[USER_PIPE] = sys_level;
    levels[SYS_PIPE] = user_level;
}

void logging_cleanup(void)
{
    if (streams[USER_PIPE] != stdin && streams[USER_PIPE] != stdout)
        fclose(streams[USER_PIPE]);

    if (streams[SYS_PIPE] != stdin && streams[SYS_PIPE] != stdout)
        fclose(streams[SYS_PIPE]);
}

void __logging_print(enum logging_level level, char const *ctx, char const *fmt,
                     ...)
{
    spinlock_lock(&lock);

    initialize_if_not_yet();

    time_t timer = time(NULL);
    struct tm *tm_info = localtime(&timer);

    strftime(stamp, sizeof(stamp), "[%H:%M:%S]", tm_info);

    va_list args;

    if (level >= levels[USER_PIPE])
    {
        va_start(args, fmt);
        pipe_into(streams[USER_PIPE], 0, fmt, args);
        va_end(args);
    }

    if (level >= levels[SYS_PIPE])
    {
        va_start(args, fmt);
        pipe_into(streams[SYS_PIPE], ctx, fmt, args);
        va_end(args);
    }

    spinlock_unlock(&lock);
}

static void initialize_if_not_yet(void)
{
    if (!streams[USER_PIPE]) streams[USER_PIPE] = stdout;
    if (!streams[SYS_PIPE]) streams[SYS_PIPE] = stderr;

    if (!levels[USER_PIPE]) levels[USER_PIPE] = LOGGING_INFO;
    if (!levels[SYS_PIPE]) levels[SYS_PIPE] = LOGGING_INFO;
}

static void pipe_into(FILE *restrict stream, char const *ctx, char const *fmt,
                      va_list args)
{
    fputs(stamp, stream);
    fputc(' ', stream);

    if (ctx)
    {
        fputc('[', stream);
        fputs(ctx, stream);
        fputc(']', stream);
        fputc(' ', stream);
    }

    vfprintf(stream, fmt, args);

    fputc('\n', stream);
    fflush(stream);
}

static FILE *open_file(char const *file, char const *default_file)
{
    if (!strcmp(file, "&1"))
    {
        return stdin;
    }
    if (!strcmp(file, "&2"))
    {
        return stdout;
    }
    FILE *fp = fopen(file, "wa");
    if (!fp) return open_file(default_file, default_file);
    return fp;
}

#include "lazylog.h"
#include <stdarg.h>
#include <stdatomic.h>
#include <time.h>

#define NANOPRINTF_IMPLEMENTATION 1
#define NANOPRINTF_VISIBILITY_STATIC 1
#include "nanoprintf.h"

static enum zlog_lvl level = ZLOG_INFO;
static char buffer[2048] = {0};
static zlog_print_fn_t *print_cb = NULL;
static void *user_arg = NULL;
static const char *strings[] = {"NOTSET", "DEBUG", "INFO",
                                "WARN",   "ERROR", "FATAL"};
static atomic_flag flag = ATOMIC_FLAG_INIT;

static inline void lock(void)
{
    while (atomic_flag_test_and_set(&flag))
        ;
}

static inline void unlock(void) { atomic_flag_clear(&flag); }

void zlog_setup(zlog_print_fn_t *print_fn, void *arg, enum zlog_lvl lvl)
{
    level = lvl;
    print_cb = print_fn;
    user_arg = arg;
}

void zlog_print(enum zlog_lvl lvl, char const *func, char const *file, int line,
                char const *fmt, ...)
{
    lock();
    if (!print_cb) goto noop;

    time_t timer = time(NULL);
    struct tm *tm_info = localtime(&timer);

    char *p = buffer;
    char *end = buffer + sizeof(buffer);

    p += strftime(buffer, sizeof(buffer), "%H:%M:%S", tm_info);

    if (lvl >= level)
    {
        p += npf_snprintf(p, end - p, " | %-5s", strings[lvl]);
        p += npf_snprintf(p, end - p, " | %s:%s:%d - ", func, file, line);

        va_list ap;
        va_start(ap, fmt);
        p += npf_vsnprintf(p, end - p, fmt, ap);
        va_end(ap);

        p += npf_snprintf(p, end - p, "%c", '\n');

        if (p >= end) p = end - 1;
    }

    if (p + 1 == end) *(p - 1) = '\n';
    *p = '\0';
    (*print_cb)(buffer, user_arg);

noop:
    unlock();
}

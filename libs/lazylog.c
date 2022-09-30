#include "lazylog.h"
#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static struct
{
    FILE *fp;
    char stamp[sizeof("24:00:00")];
    enum zlog_lvl lvl;
} self = {NULL, {0}, ZLOG_NOTSET};

static const char *strings[] = {"NOTSET", "DEBUG", "INFO",
                                "WARN",   "ERROR", "FATAL"};

static atomic_flag flag = ATOMIC_FLAG_INIT;

static inline void lock(void)
{
    while (atomic_flag_test_and_set(&flag))
        ;
}

static inline void unlock(void) { atomic_flag_clear(&flag); }

bool zlog_setup(char const *sink, enum zlog_lvl lvl)
{
    if (!strcmp(sink, "&1"))
    {
        self.fp = fdopen(1, "w");
    }
    else if (!strcmp(sink, "&2"))
    {
        self.fp = fdopen(2, "w");
    }
    else if (!sink)
    {
        self.fp = fopen("/dev/null", "w");
    }
    else
        self.fp = fopen(sink, "w");

    self.lvl = lvl;
    return !!self.fp;
}

static void ensure_open_fp(void);

void zlog_print(enum zlog_lvl lvl, char const *func, char const *file, int line,
                char const *fmt, ...)
{
    lock();
    ensure_open_fp();
    time_t timer = time(NULL);
    struct tm *tm_info = localtime(&timer);
    strftime(self.stamp, sizeof(self.stamp), "%H:%M:%S", tm_info);

    if (lvl >= self.lvl)
    {
        va_list ap;
        fprintf(self.fp, "%s | %-5s | %s:%s:%d - ", self.stamp, strings[lvl],
                func, file, line);
        va_start(ap, fmt);
        vfprintf(self.fp, fmt, ap);
        fprintf(self.fp, "\n");
        fflush(self.fp);
        va_end(ap);
    }
    unlock();
}

static void ensure_open_fp(void)
{
    if (!self.fp)
    {
        self.fp = fdopen(2, "w");
    }
}

#include "deciphon/compiler.h"
#include "deciphon/spinlock.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static char stamp[] = "[24:00:00]";
static spinlock_t lock = SPINLOCK_INIT;

void info(char const *fmt, ...)
{
    spinlock_lock(&lock);

    time_t timer = time(NULL);
    struct tm *tm_info = localtime(&timer);

    strftime(stamp, sizeof(stamp), "[%H:%M:%S]", tm_info);

    fputs(stamp, stdout);
    fputc(' ', stdout);

    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);

    fputc('\n', stdout);
    fflush(stdout);

    spinlock_unlock(&lock);
}

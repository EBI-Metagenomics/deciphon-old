#include "common/info.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

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

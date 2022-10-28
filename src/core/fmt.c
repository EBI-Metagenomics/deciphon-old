#include "core/fmt.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

static _Thread_local char buffer[4096] = {0};

char const *fmt(char const *format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof buffer, format, args);
    va_end(args);
    return buffer;
}

/* Null-terminated sequence of characters with percentage
 * from 0 to 100. String needs to have room for 4 characters. */
char *fmt_percent(char *str, int perc)
{
    assert(perc >= 0);
    assert(perc <= 100);

    *str = '1';
    str += !!(perc / 100);

    *str = (char)('0' + (perc % 100) / 10);
    str += !!(perc / 10);

    *str = (char)('0' + (perc % 100 % 10));
    ++str;

    *str = '\0';
    return str;
}

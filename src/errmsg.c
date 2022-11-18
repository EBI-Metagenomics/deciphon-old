#include "errmsg.h"
#include <stdarg.h>
#include <stdio.h>

char const *errfmt(char *dst, char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(dst, ERROR_SIZE, fmt, args);
    va_end(args);
    return dst;
}

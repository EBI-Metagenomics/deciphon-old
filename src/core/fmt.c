#include "core/fmt.h"
#include "core/compiler.h"
#include <stdarg.h>
#include <stdio.h>

static thread_local char buffer[4096] = {0};

char const *fmt(char const *format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof buffer, format, args);
    va_end(args);
    return buffer;
}

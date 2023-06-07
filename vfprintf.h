#ifndef VFPRINTF_H
#define VFPRINTF_H

#include <stdarg.h>
#include <stdio.h>

void dcp_vfprintf(FILE *restrict, char const *restrict fmt, va_list params);

#endif

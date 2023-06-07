#include "ouch.h"
#include "vfprintf.h"
#include <stdarg.h>

void ouch(char const *restrict fmt, ...)
{
  va_list params;
  va_start(params, fmt);
  dcp_vfprintf(stderr, fmt, params);
  va_end(params);
}

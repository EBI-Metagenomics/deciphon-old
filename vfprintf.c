#include "vfprintf.h"
#include "now.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

void dcp_vfprintf(FILE *restrict fp, char const *restrict fmt, va_list params)
{
  struct tm timeinfo = {0};
  now(&timeinfo);

  char buff[512] = {0};
  strftime(buff, sizeof buff, "[%d %b %y %T]: ", &timeinfo);
  vsnprintf(buff + strlen(buff), sizeof(buff) - strlen(buff), fmt, params);
  strncat(buff, "\n", sizeof(buff) - strlen(buff) - 1);

  fputs(buff, fp);
}

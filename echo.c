#include "echo.h"
#include "now.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static void echo_impl(FILE *restrict fp, char const *fmt, va_list params)
{
  struct tm timeinfo = {0};
  now(&timeinfo);

  char buff[512] = {0};
  strftime(buff, sizeof buff, "[%d %b %y %T]: ", &timeinfo);
  vsnprintf(buff + strlen(buff), sizeof(buff) - strlen(buff), fmt, params);
  strncat(buff, "\n", sizeof(buff) - strlen(buff) - 1);

  fputs(buff, fp);
}

void echof(FILE *restrict fp, char const *fmt, ...)
{
  va_list params;
  va_start(params, fmt);
  echo_impl(fp, fmt, params);
  va_end(params);
}

void echo(const char *fmt, ...)
{
  va_list params;
  va_start(params, fmt);
  echo_impl(stderr, fmt, params);
  va_end(params);
}

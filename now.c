#if !defined(_POSIX_C_SOURCE) || _POSIX_C_SOURCE < 200112L
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif

#include <assert.h>
#include <stdbool.h>
#include <time.h>

void now(struct tm *timeinfo)
{
  time_t rawtime = {0};
  bool ok = true;

  ok = time(&rawtime) != (time_t)(-1);
  assert(ok);

  ok = localtime_r(&rawtime, timeinfo) != NULL;
  assert(ok);

  (void)ok;
}

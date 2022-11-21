#include "loop/sleep.h"
#include <uv.h>

void sleep(long msec) { uv_sleep(msec); }

#include "loop/sleep.h"
#include "loop/global.h"
#include <uv.h>

void sleep(long msec) { uv_sleep(msec); }

void sleep_sync(long msec)
{
    sleep(msec);
    global_run_once();
}

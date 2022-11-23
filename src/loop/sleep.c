#include "loop/sleep.h"
#include "loop/global.h"
#include <uv.h>

void sleep(long msec)
{
    if (global_mode() == RUN_MODE_DEFAULT)
    {
        uv_sleep(msec);
        return;
    }
    uv_sleep(msec);
    global_run();
}

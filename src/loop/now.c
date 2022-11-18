#include "loop/now.h"
#include "loop/global.h"
#include <uv.h>

long now(void) { return (long)uv_now(global_loop()); }

#include "hope.h"
#include "logy.h"
#include "loop/global.h"
#include "unused.h"

int main(int argc, char *argv[])
{
    unused(argc);
    global_init(argv[0], ZLOG_DEBUG, NULL, NULL);
    COND(!global_run_once());
    return hope_status();
}

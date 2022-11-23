#include "hmmer/daemon.h"
#include "hmmer/state.h"
#include "hope.h"
#include "logy.h"
#include "loop/global.h"
#include "loop/now.h"
#include "loop/sleep.h"
#include "unused.h"

int main(int argc, char *argv[])
{
    unused(argc);
    global_init(argv[0], ZLOG_DEBUG);
    hmmerd_start("minifam.hmm");

    long deadline = now_sync() + 15000;
    while (now() < deadline && hmmerd_state() == HMMERD_BOOT)
        sleep_sync(500);

    hmmerd_stop();
    deadline = now_sync() + 15000;
    while (now_sync() < deadline && hmmerd_state() == HMMERD_ON)
        sleep_sync(500);

    hmmerd_close();
    return hope_status();
}

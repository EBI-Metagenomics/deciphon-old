#include "fs.h"
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
    COND(!hmmerd_init());
    COND(!global_run_once());

    COND(!hmmerd_start("minifam.hmm"));

    global_run_once();
    long deadline = now() + 3000;

    while (now() < deadline && hmmerd_state() == HMMERD_BOOT)
    {
        sleep(100);
        global_run_once();
    }

    EQ(hmmerd_state(), HMMERD_ON);

    return hope_status();
}

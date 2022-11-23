#include "hmmer/daemon.h"
#include "hmmer/state.h"
#include "hope.h"
#include "logy.h"
#include "loop/global.h"
#include "loop/now.h"
#include "loop/sleep.h"
#include "unused.h"

static long run_now(void);
static void run_sleep(long);

int main(int argc, char *argv[])
{
    unused(argc);
    global_init(argv[0], ZLOG_DEBUG);
    hmmerd_start("minifam.hmm");

    long deadline = run_now() + 15000;
    while (run_now() < deadline && hmmerd_state() == HMMERD_BOOT)
        run_sleep(500);

    hmmerd_stop();
    deadline = run_now() + 15000;
    while (run_now() < deadline && hmmerd_state() == HMMERD_ON)
        run_sleep(500);

    hmmerd_close();
    return hope_status();
}

static long run_now(void)
{
    global_run_once();
    return now();
}

static void run_sleep(long msec)
{
    sleep(msec);
    global_run_once();
}

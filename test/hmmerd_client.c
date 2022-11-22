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

static void startup_hmmerd(void);
static void cleanup_hmmerd(void);

int main(int argc, char *argv[])
{
    unused(argc);
    global_init(argv[0], ZLOG_DEBUG);

    startup_hmmerd();

    cleanup_hmmerd();

    return hope_status();
}

static void startup_hmmerd(void)
{
    COND(!hmmerd_init());

    EQ(hmmerd_state(), HMMERD_OFF);
    COND(!hmmerd_start("minifam.hmm"));

    long deadline = run_now() + 3000;
    while (run_now() < deadline && hmmerd_state() == HMMERD_BOOT)
    {
        run_sleep(100);
        global_run_once();
    }

    EQ(hmmerd_state(), HMMERD_ON);
    if (hope_status()) exit(hope_status());
}

static void cleanup_hmmerd(void)
{
    hmmerd_stop();

    long deadline = run_now() + 5000;
    while (run_now() < deadline && hmmerd_state() == HMMERD_ON)
    {
        run_sleep(100);
        global_run_once();
    }

    EQ(hmmerd_state(), HMMERD_OFF);

    hmmerd_close();
    COND(!global_run_once());
    if (hope_status()) exit(hope_status());
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

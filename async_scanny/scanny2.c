#include "fs.h"
#include "hmmer/client.h"
#include "hmmer/server.h"
#include "hmmer/state.h"
#include "logy.h"
#include "loop/global.h"
#include "loop_while.h"
#include "scan/scan.h"
#include "unused.h"

static void startup_hmmerd(void);
static void cleanup_hmmerd(void);

static void startup_hmmerc(int nstreams);
static void cleanup_hmmerc(void);

int main(int argc, char *argv[])
{
    unused(argc);
    global_init(argv[0], ZLOG_DEBUG);
    global_set_mode(RUN_MODE_NOWAIT);
    startup_hmmerd();
    startup_hmmerc(1);

    struct scan_cfg cfg = {1, 10., true, false};
    scan_init(cfg);
    scan_setup(argv[2], argv[1]);
    scan_run();
    scan_finishup();
    scan_cleanup();

    cleanup_hmmerc();
    cleanup_hmmerd();
    return 0;
}

static void startup_hmmerd(void)
{
    hmmer_server_start("minifam.hmm");
    loop_while(20000, hmmer_server_state() == HMMERD_BOOT);
    hmmer_server_state();
}

static void cleanup_hmmerd(void)
{
    hmmer_server_stop();
    loop_while(15000, hmmer_server_state() == HMMERD_ON);
    hmmer_server_state();

    hmmer_server_close();
}

static void startup_hmmerc(int nstreams)
{
    hmmer_client_start(nstreams, now() + 5000);
    loop_while(500, true);
}

static void cleanup_hmmerc(void)
{
    hmmer_client_stop();
    loop_while(500, true);
}

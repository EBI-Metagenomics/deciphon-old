#include "scan/scan.h"
#include "fs.h"
#include "hmmer/client.h"
#include "hmmer/server.h"
#include "hmmer/state.h"
#include "hope.h"
#include "logy.h"
#include "loop/global.h"
#include "loop_while.h"
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
    eq(scan_setup("minifam.dcp", "consensus.json"), 0);
    eq(scan_run(), 0);
    eq(scan_finishup(), 0);
    long chk = 0;
    eq(fs_cksum("prod/match.tsv", FS_FLETCHER16, &chk), 0);
    eq(chk, 21919);
    scan_cleanup();

    cleanup_hmmerc();
    cleanup_hmmerd();
    return hope_status();
}

static void startup_hmmerd(void)
{
    eq(hmmer_server_start("minifam.hmm"), 0);
    loop_while(20000, hmmer_server_state() == HMMERD_BOOT);
    eq(hmmer_server_state(), HMMERD_ON);
}

static void cleanup_hmmerd(void)
{
    hmmer_server_stop();
    loop_while(15000, hmmer_server_state() == HMMERD_ON);
    eq(hmmer_server_state(), HMMERD_OFF);

    hmmer_server_close();
}

static void startup_hmmerc(int nstreams)
{
    eq(hmmer_client_start(nstreams, now() + 5000), 0);
    loop_while(500, true);
}

static void cleanup_hmmerc(void)
{
    hmmer_client_stop();
    loop_while(500, true);
}

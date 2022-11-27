#include "hmmer/server.h"
#include "hmmer/state.h"
#include "hope.h"
#include "logy.h"
#include "loop/global.h"
#include "loop/now.h"
#include "loop/sleep.h"
#include "loop_while.h"
#include "unused.h"
#include <uv.h>

int main(int argc, char *argv[])
{
    unused(argc);
    global_init(argv[0], ZLOG_DEBUG);
    global_set_mode(RUN_MODE_NOWAIT);

    eq(hmmer_server_start("minifam.hmm"), 0);
    loop_while(15000, hmmer_server_state() == HMMERD_BOOT);
    eq(hmmer_server_state(), HMMERD_ON);

    hmmer_server_stop();
    loop_while(15000, hmmer_server_state() == HMMERD_ON);
    eq(hmmer_server_state(), HMMERD_OFF);

    hmmer_server_close();
    return hope_status();
}

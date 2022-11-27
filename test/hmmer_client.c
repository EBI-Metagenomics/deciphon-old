#include "fs.h"
#include "helper.h"
#include "hmmer/client.h"
#include "hmmer/result.h"
#include "hmmer/server.h"
#include "hmmer/state.h"
#include "hope.h"
#include "logy.h"
#include "loop/global.h"
#include "loop/now.h"
#include "loop/sleep.h"
#include "loop_while.h"
#include "ross.h"
#include "unused.h"

static void test_connection(void);
static void test_put_pop(void);

int main(int argc, char *argv[])
{
    unused(argc);
    global_init(argv[0], ZLOG_DEBUG);
    global_set_mode(RUN_MODE_NOWAIT);

    eq(hmmer_server_start("ross.5.hmm"), 0);
    loop_while(15000, hmmer_server_state() == HMMERD_BOOT);
    eq(hmmer_server_state(), HMMERD_ON);

    test_connection();
    test_put_pop();

    hmmer_server_stop();
    loop_while(15000, hmmer_server_state() == HMMERD_ON);
    eq(hmmer_server_state(), HMMERD_OFF);

    hmmer_server_close();
    return hope_status();
}

static void test_connection(void)
{
    eq(hmmer_client_start(1, now() + 5000), 0);
    loop_while(1000, true);
    hmmer_client_stop();
    loop_while(1000, true);
}

static void test_put_pop(void)
{
    eq(hmmer_client_start(1, now() + 50000), 0);
    loop_while(1000, true);

    eq(hmmer_client_put(0, 0, ross, now() + 10000), 0);
    loop_while(1000, true);

    struct hmmer_result *r = NULL;
    eq(hmmer_result_new(&r), 0);

    eq(hmmer_client_pop(0, r), 0);
    close(-53.808984215028, hmmer_result_evalue_ln(r));
    loop_while(1000, true);

    hmmer_result_del(r);

    hmmer_client_stop();
    loop_while(1000, true);
}

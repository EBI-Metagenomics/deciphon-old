#include "fs.h"
#include "helper.h"
#include "hmmer/client.h"
#include "hmmer/daemon.h"
#include "hmmer/state.h"
#include "hope.h"
#include "logy.h"
#include "loop/global.h"
#include "loop/now.h"
#include "loop/sleep.h"
#include "loop_while.h"
#include "unused.h"

static char const *seq_new(void);
static void test_connection(void);
static void test_put_pop(void);

int main(int argc, char *argv[])
{
    unused(argc);
    global_init(argv[0], ZLOG_DEBUG);
    global_set_mode(RUN_MODE_NOWAIT);

    EQ(hmmerd_start("ross.5.hmm"), 0);
    loop_while(15000, hmmerd_state() == HMMERD_BOOT);
    EQ(hmmerd_state(), HMMERD_ON);

    test_connection();
    test_put_pop();

    hmmerd_stop();
    loop_while(15000, hmmerd_state() == HMMERD_ON);
    EQ(hmmerd_state(), HMMERD_OFF);

    hmmerd_close();
    return hope_status();
}

static void test_connection(void)
{
    EQ(hmmerc_start(1, now() + 5000), 0);
    loop_while(1000, true);
    hmmerc_stop();
    loop_while(1000, true);
}

static void test_put_pop(void)
{
    char const *seq = seq_new();
    EQ(hmmerc_start(1, now() + 50000), 0);
    loop_while(1000, true);

    EQ(hmmerc_put(0, seq, now() + 10000), 0);
    loop_while(1000, true);

    double ln_evalue = 0;
    EQ(hmmerc_pop(0, &ln_evalue), 0);
    CLOSE(-53.808984215028, ln_evalue);
    loop_while(1000, true);

    hmmerc_stop();
    loop_while(1000, true);
    free((void *)seq);
}

static char const *seq_new(void)
{
    long size = 0;
    unsigned char *data = NULL;
    EQ(fs_readall("ross.fna", &size, &data), 0);
    return append_char(size, (char *)data, '\0');
}

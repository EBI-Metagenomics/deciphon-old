#include "fs.h"
#include "h3c/h3c.h"
#include "hmmer/client.h"
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

char *append_char(size_t n, char *dst, char c);

int main(int argc, char *argv[])
{
    unused(argc);
    global_init(argv[0], ZLOG_DEBUG);
    fprintf(stderr, "Ponto 0\n");
    global_run_once();
    global_run_once();
    startup_hmmerd();
    fprintf(stderr, "Ponto 1\n");
    global_run_once();
    global_run_once();

    long size = 0;
    unsigned char *data = NULL;
    EQ(fs_readall("ross.fna", &size, &data), 0);
    char const *seq = append_char(size, (char *)data, '\0');
    fprintf(stderr, "Ponto 2\n");
    global_run_once();
    global_run_once();

    EQ(hmmerc_start(1, run_now() + 5000), 0);
    fprintf(stderr, "Ponto 3\n");
    global_run_once();
    global_run_once();
    EQ(hmmerc_put(0, seq, run_now() + 15000), 0);
    fprintf(stderr, "Ponto 4\n");
    global_run_once();
    global_run_once();
    double ln_evalue = 0;
    EQ(hmmerc_pop(0, &ln_evalue), 0);
    fprintf(stderr, "Ponto 5\n");
    global_run_once();
    global_run_once();
    hmmerc_stop();
    fprintf(stderr, "Ponto 6\n");
    global_run_once();
    global_run_once();

    free((void *)seq);

    cleanup_hmmerd();
    return hope_status();
}

static void startup_hmmerd(void)
{
    fprintf(stderr, "Startup daemon 0\n");
    COND(!hmmerd_init());

    fprintf(stderr, "Startup daemon 1\n");
    EQ(hmmerd_state(), HMMERD_OFF);
    COND(!hmmerd_start("ross.5.hmm"));
    fprintf(stderr, "Startup daemon 2\n");

    long deadline = run_now() + 15000;
    while (run_now() < deadline && hmmerd_state() == HMMERD_BOOT)
    {
        fprintf(stderr, "Startup daemon 3\n");
        run_sleep(500);
    }

    fprintf(stderr, "Startup daemon 4\n");
    EQ(hmmerd_state(), HMMERD_ON);
    if (hope_status()) exit(hope_status());
}

static void cleanup_hmmerd(void)
{
    hmmerd_stop();

    long deadline = run_now() + 5000;
    while (run_now() < deadline && hmmerd_state() == HMMERD_ON)
    {
        run_sleep(500);
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

char *append_char(size_t n, char *dst, char c)
{
    char *t = realloc(dst, n + 1);
    if (!t) exit(EXIT_FAILURE);
    t[n] = c;
    return t;
}

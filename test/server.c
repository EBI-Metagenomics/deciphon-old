#include "deciphon/server/server.h"
#include "deciphon/db/db.h"
#include "deciphon/sched/api.h"
#include "hope.h"
#include "imm/imm.h"
#include <curl/curl.h>

static struct sched_hmm hmm = {0};
static struct api_rc error = {0};

void test_server(void);

static bool is_sched_reachable(void)
{
    enum rc rc = api_init(SCHED_API_URL);
    if (rc) return false;
    bool reachable = api_is_reachable();
    api_cleanup();
    return reachable;
}

int main(void)
{
    if (!is_sched_reachable())
    {
        fprintf(stderr, "Scheduler is not reachable ");
        fprintf(stderr, "so we canno't really test it.\n");
        return 1;
    }
    test_server();
    return hope_status();
}

void test_server(void)
{
    EQ(api_init(SCHED_API_URL), RC_OK);
    EQ(api_wipe(), RC_OK);

    EQ(api_upload_hmm(ASSETS "/PF02545.hmm", &hmm, &error), RC_OK);

    api_cleanup();

    struct server_cfg cfg = SERVER_CFG_INIT;
    // cfg.single_run = true;
    EQ(server_init(SCHED_API_URL, cfg), RC_OK);
    EQ(server_run(), RC_OK);
    server_cleanup();
}

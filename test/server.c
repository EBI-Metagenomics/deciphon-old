#include "deciphon/server/server.h"
#include "deciphon/db/db.h"
#include "deciphon/sched/api.h"
#include "hope/hope.h"
#include "imm/imm.h"
#include <curl/curl.h>

void test_server(void);

int main(void)
{
    test_server();
    return hope_status();
}

void test_server(void)
{
    EQ(api_init(SCHED_API_URL), RC_OK);
    EQ(api_wipe(), RC_OK);
    api_cleanup();

    struct server_cfg cfg = SERVER_CFG_INIT;
    cfg.single_run = true;
    EQ(server_init(SCHED_API_URL, cfg), RC_OK);
    EQ(server_run(), RC_OK);
    server_cleanup();
}

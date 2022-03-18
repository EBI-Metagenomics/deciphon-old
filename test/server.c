#include "deciphon/server/server.h"
#include "deciphon/db/db.h"
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
    EQ(rest_open(SCHED_API_URL), RC_OK);
    EQ(rest_wipe(), RC_OK);

    struct sched_job job = {0};
    struct rest_error error = {0};

    EQ(rest_next_pend_job(&job, &error), RC_END);
    EQ(error.rc, RC_OK);
    EQ(error.msg, "");
    EQ(job.id, 0);

    EQ(rest_testing_data(&error), RC_OK);
    EQ(error.rc, RC_OK);
    EQ(error.msg, "");

    rest_close();

    EQ(server_run(true, 1, SCHED_API_URL), RC_OK);
}

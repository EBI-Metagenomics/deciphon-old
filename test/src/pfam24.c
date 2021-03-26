#include "cass/cass.h"
#include "dcp/dcp.h"

static struct dcp_server* server_setup(void);
static void               server_teardown(struct dcp_server* server);

int main(void)
{
    struct dcp_server* server = server_setup();

    bool                calc_loglik = true;
    bool                calc_null = true;
    bool                multiple_hits = true;
    bool                hmmer3_compat = true;
    struct dcp_task_cfg cfg = {calc_loglik, calc_null, multiple_hits, hmmer3_compat, false};
    struct dcp_task*    task = dcp_task_create(cfg);

    dcp_task_add_seq(task, "ACT");
    dcp_server_add(server, task);

    /* while (!dcp_task_end(task)) { */

    /*     struct dcp_results* results = dcp_task_read(task); */
    /*     if (!results) */
    /*         continue; */

    /*     dcp_server_recyle(server, results); */
    /* } */
    dcp_task_destroy(task);

    server_teardown(server);

    return cass_status();
}

static struct dcp_server* server_setup(void)
{
    char const*        filepath = "/Users/horta/tmp/pfam24.dcp";
    struct dcp_server* server = dcp_server_create(filepath);

    dcp_server_start(server);
    return server;
}

static void server_teardown(struct dcp_server* server)
{
    dcp_server_stop(server);
    dcp_server_join(server);
    dcp_server_destroy(server);
}

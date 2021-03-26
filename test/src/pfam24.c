#include "cass/cass.h"
#include "dcp/dcp.h"

int main(void)
{
    char const*        filepath = "/Users/horta/tmp/pfam24.dcp";
    struct dcp_server* server = dcp_server_create(filepath);

    dcp_server_start(server);

    bool calc_loglik = true;
    bool calc_null = true;
    bool multiple_hits = true;
    bool hmmer3_compat = true;
    struct dcp_task_cfg cfg = {calc_loglik, calc_null, multiple_hits, hmmer3_compat, false};
    struct dcp_task*    task = dcp_task_create(cfg);

    dcp_task_add_seq(task, "ACT");
    dcp_server_add(server, task);

    printf("Ponto 1\n");
    fflush(stdout);
    while (!dcp_task_end(task)) {

        struct dcp_results* results = dcp_task_read(task);
        if (!results)
            continue;

        dcp_server_recyle(server, results);
    }
    /* dcp_task_destroy(task); */
    printf("Ponto 2\n");
    fflush(stdout);
    return 0;

    dcp_server_stop(server);
    dcp_server_join(server);
    dcp_server_destroy(server);
    return cass_status();
}

#include "cass/cass.h"
#include "dcp/dcp.h"

void test_tasks(void);

int main(void)
{
    test_tasks();
    return cass_status();
}

void test_tasks(void)
{
    char const* filepath = "/Users/horta/tmp/pfam24.dcp";

    struct dcp_server* server = dcp_server_create(filepath);
    cass_not_null(server);

    cass_equal(dcp_server_start(server), 0);

    struct dcp_task_cfg cfg = {true, true, false, false, false};
    struct dcp_task*    task = dcp_task_create(cfg);

    /* >Leader_Thr-sample1 */
    /* MRRNRMIATIITTTITTLGAG */
    dcp_task_add_seq(task, "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG");
    dcp_server_add_task(server, task);

    while (!dcp_task_end(task)) {
        struct dcp_results* results = dcp_task_read(task);
        if (!results) {
            dcp_sleep(50);
            continue;
        }

        for (uint16_t i = 0; i < dcp_results_size(results); ++i) {
            struct dcp_result const* r = dcp_results_get(results, i);
            uint32_t                 seqid = dcp_result_seqid(r);
            uint32_t                 profid = dcp_result_profid(r);

            for (unsigned j = 0; j < 2; ++j) {

                enum dcp_model m = dcp_models[j];
                unsigned       k = 3 * profid + seqid;
                /* printf("%f\n", dcp_result_loglik(r, m)); */

                /* cass_close(dcp_result_loglik(r, m), logliks[m][k]); */

                /* struct dcp_metadata const* mt = dcp_server_metadata(server, profid); */

                struct dcp_string const* path = dcp_result_path(r, m);
                if (j == 0)
                    printf("%.*s\n", dcp_string_size(path), dcp_string_data(path));

                struct dcp_string const* codons = dcp_result_codons(r, m);
                if (j == 0)
                    printf("%.*s\n", dcp_string_size(codons), dcp_string_data(codons));
                /* cass_equal(strcmp(dcp_string_data(path), paths[m][k]), 0); */

                /* ncomp++; */
            }
        }
        dcp_server_free_results(server, results);
    }

    dcp_server_free_task(server, task);
    dcp_server_stop(server);

    cass_equal(dcp_server_destroy(server), 0);
}

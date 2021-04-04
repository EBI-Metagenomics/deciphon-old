#include "cass/cass.h"
#include "dcp/dcp.h"
#include "pfam24_data.h"
#include "pparr.h"

#define LOGLIKS(i) SHAPE(24, 2, 2, 2, 2, i)
#define PATHS(i) SHAPE(24, 2, 2, 2, 2, i)
#define CODONS(i) SHAPE(24, 2, 2, 2, 2, i)

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

    struct dcp_task_cfg cfgs[4] = {{true, true, false, false, true},
                                   {true, true, true, false, true},
                                   {true, true, false, true, true},
                                   {true, true, true, true, true}};

    for (unsigned k = 0; k < 4; ++k) {
        struct dcp_task* task = dcp_task_create(cfgs[k]);
        /* >Leader_Thr-sample1 */
        /* MRRNRMIATIITTTITTLGAG */
        dcp_task_add_seq(task, "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG");
        dcp_task_add_seq(task,
                         "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCGCAGATGCGCCGCAACCGCATGATTGCGACCA"
                         "TTATTACCACCACCATTACCACCCTGGGCGCG");
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
                    enum dcp_model      m = dcp_models[j];
                    struct dcp_task_cfg cfg = dcp_task_cfg(task);
                    bool                h3compat = cfg.hmmer3_compat;
                    bool                mhits = cfg.multiple_hits;
#define GET_SHAPE(i) LOGLIKS(i)
                    cass_close(dcp_result_loglik(r, m), logliks[_(profid, seqid, j, h3compat, mhits)]);
#undef GET_SHAPE

#define GET_SHAPE(i) PATHS(i)
                    char const* path = dcp_string_data(dcp_result_path(r, m));
                    cass_equal(strcmp(path, paths[_(profid, seqid, j, h3compat, mhits)]), 0);
#undef GET_SHAPE

#define GET_SHAPE(i) CODONS(i)
                    char const* codon = dcp_string_data(dcp_result_codons(r, m));
                    cass_equal(strcmp(codon, codons[_(profid, seqid, j, h3compat, mhits)]), 0);
#undef GET_SHAPE
                }
            }
            dcp_server_free_results(server, results);
        }
        dcp_server_free_task(server, task);
    }

    dcp_server_stop(server);

    cass_equal(dcp_server_destroy(server), 0);
}

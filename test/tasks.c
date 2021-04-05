#include "cass/cass.h"
#include "dcp/dcp.h"
#include "pfam24_results.h"

void test_pfam24_sample(void);
void test_wrong_alphabet(void);

int main(void)
{
    test_pfam24_sample();
    test_wrong_alphabet();
    return cass_status();
}

void test_pfam24_sample(void)
{
    char const* filepath = PFAM24_FILEPATH;

    struct dcp_server* server = dcp_server_create(filepath);
    cass_not_null(server);

    cass_equal(dcp_server_start(server), 0);

    struct dcp_task_cfg cfgs[4] = {
        {true, true, false, false}, {true, true, true, false}, {true, true, false, true}, {true, true, true, true}};

    for (unsigned k = 0; k < 4; ++k) {
        struct dcp_task* task = dcp_task_create(cfgs[k]);

        /* >Leader_Thr-sample1 */
        /* MRRNRMIATIITTTITTLGAG */
#define LEADER_THR "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG"
        dcp_task_add_seq(task, LEADER_THR);
        dcp_task_add_seq(task, LEADER_THR "CAG" LEADER_THR);
#undef LEADER_THR

        dcp_server_add_task(server, task);

        while (!dcp_task_end(task)) {
            struct dcp_result const* r = dcp_task_read(task);
            if (!r) {
                dcp_sleep(10);
                continue;
            }

            uint32_t seqid = dcp_result_seqid(r);
            uint32_t profid = dcp_result_profid(r);

            for (unsigned j = 0; j < 2; ++j) {
                enum dcp_model      m = dcp_models[j];
                struct dcp_task_cfg cfg = dcp_task_cfg(task);
                bool                h3compat = cfg.hmmer3_compat;
                bool                mhits = cfg.multiple_hits;
                cass_close(dcp_result_loglik(r, m), T(logliks, profid, seqid, j, h3compat, mhits));

                char const* path = dcp_string_data(dcp_result_path(r, m));
                cass_equal(strcmp(path, T(paths, profid, seqid, j, h3compat, mhits)), 0);

                char const* codon = dcp_string_data(dcp_result_codons(r, m));
                cass_equal(strcmp(codon, T(codons, profid, seqid, j, h3compat, mhits)), 0);
            }
            dcp_server_free_result(server, r);
        }
        dcp_server_free_task(server, task);
    }

    dcp_server_stop(server);
    dcp_server_join(server);

    cass_equal(dcp_server_destroy(server), 0);
}

void test_wrong_alphabet(void)
{
    char const* filepath = PFAM24_FILEPATH;

    struct dcp_server* server = dcp_server_create(filepath);
    cass_not_null(server);

    cass_equal(dcp_server_start(server), 0);

    struct dcp_task_cfg cfg = {true, true, false, false};
    struct dcp_task*    task = dcp_task_create(cfg);
    dcp_task_add_seq(task, "ATGCK");
    dcp_server_add_task(server, task);

    while (!dcp_task_end(task)) {
        struct dcp_result const* r = dcp_task_read(task);
        if (!r) {
            dcp_sleep(10);
            continue;
        }
        cass_equal(dcp_result_error(r), 1);
        dcp_server_free_result(server, r);
    }
    dcp_server_free_task(server, task);

    dcp_server_stop(server);
    dcp_server_join(server);

    cass_equal(dcp_server_destroy(server), 0);
}

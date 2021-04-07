#include "cass/cass.h"
#include "dcp/dcp.h"
#include <stdlib.h>
#include <string.h>
/* #include "pfam24_results.h" */

void test_very_long_seq(void);

int main(void)
{
    test_very_long_seq();
    return cass_status();
}

void test_very_long_seq(void)
{
    char const* filepath = PFAM24_FILEPATH;

    struct dcp_server* server = dcp_server_create(filepath);
    cass_not_null(server);

    cass_equal(dcp_server_start(server), 0);

    struct dcp_task_cfg cfg = {true, true, true, false};
    struct dcp_task* task = dcp_task_create(cfg);

    unsigned   rep = 10000;
    char const sep_str[] = "CAG";
    char const leader_thr[] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG";
    char*      seq = malloc(strlen(leader_thr) * rep + strlen(sep_str) * (rep - 1) + 1);
    size_t     k = 0;
    for (unsigned i = 0; i < rep; ++i) {
        if (i > 0) {
            for (unsigned j = 0; j < strlen(sep_str); ++j)
                seq[k++] = sep_str[j];
        }
        for (unsigned j = 0; j < strlen(leader_thr); ++j)
            seq[k++] = leader_thr[j];
    }
    seq[k] = '\0';
    printf("Size: %lu\n", strlen(seq));

    dcp_task_add_seq(task, seq);
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
            bool                h3compat = cfg.hmmer3_compat;
            bool                mhits = cfg.multiple_hits;
            /* cass_close(dcp_result_loglik(r, m), T(logliks, profid, seqid, j, h3compat, mhits)); */

            /* char const* path = dcp_string_data(dcp_result_path(r, m)); */
            /* cass_equal(strcmp(path, T(paths, profid, seqid, j, h3compat, mhits)), 0); */

            /* char const* codon = dcp_string_data(dcp_result_codons(r, m)); */
            /* cass_equal(strcmp(codon, T(codons, profid, seqid, j, h3compat, mhits)), 0); */
        }
        dcp_server_free_result(server, r);
    }
    dcp_server_free_task(server, task);

    dcp_server_stop(server);
    dcp_server_join(server);

    cass_equal(dcp_server_destroy(server), 0);

    free(seq);
}

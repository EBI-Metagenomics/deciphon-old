#include "hope/hope.h"
#include "job.h"
#include "server.h"

void test_pfam24_sample(unsigned num_threads);
/* void test_wrong_alphabet(void); */

struct sched_job sched_job = {0};
struct sched_prod sched_prod = {0};

int main(void)
{
    test_pfam24_sample(1);
    /* test_wrong_alphabet(); */
    return hope_status();
}

void test_pfam24_sample(unsigned num_threads)
{
    // char const *db_path = ASSETS "/pfam24.dcp";
    char const *db_path = "/Users/horta/code/deciphon/build/pfam24_0.1.dcp";
    char const *sched_path = TMPDIR "/pfam24.sched";

    remove(sched_path);
    EQ(server_open(sched_path, num_threads), RC_DONE);

    int64_t db_id = 0;
    EQ(server_add_db(db_path, &db_id), RC_DONE);
    EQ(db_id, 1);

    struct job job = {0};
    job_init(&job, db_id);

    struct seq seq[2] = {0};

    /* >Leader_Thr-sample1 */
    /* MRRNRMIATIITTTITTLGAG */
#define LEADER_THR                                                             \
    "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG"
    seq_init(seq + 0, "Leader_Thr-sample1", imm_str(LEADER_THR));
    seq_init(seq + 1, "Leader_Thr-sample1_multihit",
             imm_str(LEADER_THR "CAG" LEADER_THR));
#undef LEADER_THR

    job_add_seq(&job, seq + 0);
    job_add_seq(&job, seq + 1);

    EQ(server_submit_job(&job), RC_DONE);
    COND(job.id > 0);

    server_set_lrt_threshold(10.f);
    EQ(server_run(true), RC_DONE);

    sched_job.id = job.id;
    EQ(server_get_sched_job(&sched_job), RC_DONE);
    EQ(sched_job.error, "");
    EQ(sched_job.multi_hits, true);
    EQ(sched_job.hmmer3_compat, false);
    EQ(sched_job.state, "done");

    EQ(server_close(), RC_DONE);

#if 0
    struct dcp_task_cfg cfgs[4] = {{true, true, false, false},
                                   {true, true, true, false},
                                   {true, true, false, true},
                                   {true, true, true, true}};

    for (unsigned k = 0; k < 4; ++k)
    {
        struct dcp_task *task = dcp_task_create(cfgs[k]);

        /* >Leader_Thr-sample1 */
        /* MRRNRMIATIITTTITTLGAG */
#define LEADER_THR                                                             \
    "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG"
        dcp_task_add_seq(task, LEADER_THR);
        dcp_task_add_seq(task, LEADER_THR "CAG" LEADER_THR);
#undef LEADER_THR

        dcp_server_add_task(server, task);

        while (!dcp_task_end(task))
        {
            struct dcp_result const *r = dcp_task_read(task);
            if (!r)
            {
                dcp_sleep(10);
                continue;
            }

            uint32_t seqid = dcp_result_seqid(r);
            uint32_t profid = dcp_result_profid(r);

            for (unsigned j = 0; j < 2; ++j)
            {
                enum dcp_model m = dcp_models[j];
                struct dcp_task_cfg cfg = dcp_task_cfg(task);
                bool h3compat = cfg.hmmer3_compat;
                bool mhits = cfg.multiple_hits;
                cass_close(dcp_result_loglik(r, m),
                           T(logliks, profid, seqid, j, h3compat, mhits));

                char const *path = dcp_string_data(dcp_result_path(r, m));
                cass_equal(
                    strcmp(path, T(paths, profid, seqid, j, h3compat, mhits)),
                    0);

                char const *codon = dcp_string_data(dcp_result_codons(r, m));
                cass_equal(
                    strcmp(codon, T(codons, profid, seqid, j, h3compat, mhits)),
                    0);
            }
            dcp_server_free_result(server, r);
        }
        dcp_server_free_task(server, task);
    }

    dcp_server_stop(server);
    dcp_server_join(server);

    cass_equal(dcp_server_destroy(server), 0);
#endif
}

#if 0
void test_wrong_alphabet(void)
{
    char const *filepath = PFAM24_FILEPATH;

    struct dcp_server *server = dcp_server_create(filepath);
    cass_not_null(server);

    cass_equal(dcp_server_start(server), 0);

    struct dcp_task_cfg cfg = {true, true, false, false};
    struct dcp_task *task = dcp_task_create(cfg);
    dcp_task_add_seq(task, "ATGCK");
    dcp_server_add_task(server, task);

    while (!dcp_task_end(task))
    {
        struct dcp_result const *r = dcp_task_read(task);
        if (!r)
        {
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
#endif

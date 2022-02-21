#include "hope/hope.h"
#include "job.h"
#include "server.h"

void test_pfam24_sample(bool multi_hits, bool hmmer3_compat,
                        unsigned num_threads);
/* void test_wrong_alphabet(void); */

struct sched_job sched_job = {0};
struct sched_prod prod = {0};

int main(void)
{
    test_pfam24_sample(true, false, 1);
    test_pfam24_sample(false, false, 1);
    /* test_wrong_alphabet(); */
    return hope_status();
}

void test_pfam24_sample(bool multi_hits, bool hmmer3_compat,
                        unsigned num_threads)
{
    char const *db_path = ASSETS "/pfam24.dcp";
    char const *sched_path = TMPDIR "/pfam24.sched";

    remove(sched_path);
    EQ(server_open(sched_path, num_threads), DCP_OK);

    int64_t db_id = 0;
    EQ(server_add_db(db_path, &db_id), DCP_OK);
    EQ(db_id, 1);

    struct job job = {0};
    job_init(&job, db_id);
    job.multi_hits = multi_hits;
    job.hmmer3_compat = hmmer3_compat;

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

    EQ(server_submit_job(&job), DCP_OK);
    COND(job.id > 0);

    server_set_lrt_threshold(10.f);
    EQ(server_run(true), DCP_OK);

    sched_job.id = job.id;
    EQ(server_get_sched_job(&sched_job), DCP_OK);
    EQ(sched_job.error, "");
    EQ(sched_job.multi_hits, multi_hits);
    EQ(sched_job.hmmer3_compat, hmmer3_compat);
    EQ(sched_job.state, "done");

    enum rc rc = DCP_OK;
    prod.id = 0;
    prod.job_id = sched_job.id;
    while ((rc = server_next_sched_prod(&sched_job, &prod)) == RC_NEXT)
    {
        // printf("%s %s %s %s \n", prod.abc_name, prod.profile_name,
        //        prod.profile_typeid, prod.version);
        // printf("%.10f %.10f \n", prod.alt_loglik, prod.null_loglik);
        // printf("%lld %s \n", prod.seq_id, prod.match);

        if (prod.seq_id == 1 && strcmp(prod.profile_name, "PF08254.12") == 0 &&
            job.multi_hits && !job.hmmer3_compat)
        {
            CLOSE(prod.alt_loglik, -68.01567077637);
            CLOSE(prod.null_loglik, -84.16704559326);
            EQ(prod.profile_typeid, "protein");
            EQ(prod.abc_name, "dna");
            EQ(prod.match,
               ",S,,;,B,,;ATG,M1,ATG,M;CGC,M2,CGC,R;CGC,M3,CGC,R;AAC,M4,AAC,N;"
               "CGC,M5,CGC,R;ATG,M6,ATG,M;ATT,M7,ATT,I;GCG,M8,GCG,A;ACC,M9,ACC,"
               "T;ATT,M10,ATT,I;ATT,M11,ATT,I;ACC,M12,ACC,T;ACC,M13,ACC,T;ACC,"
               "M14,ACC,T;ATT,M15,ATT,I;ACC,M16,ACC,T;ACC,M17,ACC,T;,E,,;CTG,C,"
               "CTG,L;GGC,C,GGC,G;GCG,C,GCG,A;,T,,");
        }
        else if (prod.seq_id == 2 &&
                 strcmp(prod.profile_name, "PF08254.12") == 0 &&
                 job.multi_hits && !job.hmmer3_compat)
        {
            CLOSE(prod.alt_loglik, -138.90509033203);
            CLOSE(prod.null_loglik, -171.99272155762);
            EQ(prod.profile_typeid, "protein");
            EQ(prod.abc_name, "dna");
            EQ(prod.match,
               ",S,,;,B,,;ATG,M1,ATG,M;CGC,M2,CGC,R;CGC,M3,CGC,R;AAC,M4,AAC,N;"
               "CGC,M5,CGC,R;ATG,M6,ATG,M;ATT,M7,ATT,I;GCG,M8,GCG,A;ACC,M9,ACC,"
               "T;ATT,M10,ATT,I;ATT,M11,ATT,I;ACC,M12,ACC,T;ACC,M13,ACC,T;ACC,"
               "M14,ACC,T;ATT,M15,ATT,I;ACC,M16,ACC,T;ACC,M17,ACC,T;,E,,;CTG,J,"
               "CTG,L;GGC,J,GGC,G;GCG,J,GCG,A;CAG,J,CAG,Q;,B,,;ATG,M1,ATG,M;"
               "CGC,M2,CGC,R;CGC,M3,CGC,R;AAC,M4,AAC,N;CGC,M5,CGC,R;ATG,M6,ATG,"
               "M;ATT,M7,ATT,I;GCG,M8,GCG,A;ACC,M9,ACC,T;ATT,M10,ATT,I;ATT,M11,"
               "ATT,I;ACC,M12,ACC,T;ACC,M13,ACC,T;ACC,M14,ACC,T;ATT,M15,ATT,I;"
               "ACC,M16,ACC,T;ACC,M17,ACC,T;,E,,;CTG,C,CTG,L;GGC,C,GGC,G;GCG,C,"
               "GCG,A;,T,,");
        }
        else if (prod.seq_id == 1 &&
                 strcmp(prod.profile_name, "PF08254.12") == 0 &&
                 !job.multi_hits && !job.hmmer3_compat)
        {
            CLOSE(prod.alt_loglik, -68.05345916748);
            CLOSE(prod.null_loglik, -84.16704559326);
            EQ(prod.profile_typeid, "protein");
            EQ(prod.abc_name, "dna");
            EQ(prod.match,
               ",S,,;,B,,;ATG,M1,ATG,M;CGC,M2,CGC,R;CGC,M3,CGC,R;AAC,M4,AAC,N;"
               "CGC,M5,CGC,R;ATG,M6,ATG,M;ATT,M7,ATT,I;GCG,M8,GCG,A;ACC,M9,ACC,"
               "T;ATT,M10,ATT,I;ATT,M11,ATT,I;ACC,M12,ACC,T;ACC,M13,ACC,T;ACC,"
               "M14,ACC,T;ATT,M15,ATT,I;ACC,M16,ACC,T;ACC,M17,ACC,T;,E,,;CTG,C,"
               "CTG,L;GGC,C,GGC,G;GCG,C,GCG,A;,T,,");
        }
        else if (prod.seq_id == 2 &&
                 strcmp(prod.profile_name, "PF08254.12") == 0 &&
                 !job.multi_hits && !job.hmmer3_compat)
        {
            CLOSE(prod.alt_loglik, -157.56034851074);
            CLOSE(prod.null_loglik, -171.99272155762);
            EQ(prod.profile_typeid, "protein");
            EQ(prod.abc_name, "dna");
            EQ(prod.match,
               ",S,,;,B,,;ATG,M1,ATG,M;CGC,M2,CGC,R;CGC,M3,CGC,R;AAC,M4,AAC,N;"
               "CGC,M5,CGC,R;ATG,M6,ATG,M;ATT,M7,ATT,I;GCG,M8,GCG,A;ACC,M9,ACC,"
               "T;ATT,M10,ATT,I;ATT,M11,ATT,I;ACC,M12,ACC,T;ACC,M13,ACC,T;ACC,"
               "M14,ACC,T;ATT,M15,ATT,I;ACC,M16,ACC,T;ACC,M17,ACC,T;,E,,;CTG,C,"
               "CTG,L;GGC,C,GGC,G;GCG,C,GCG,A;CAG,C,CAG,Q;ATG,C,ATG,M;CGC,C,"
               "CGC,R;CGC,C,CGC,R;AAC,C,AAC,N;CGC,C,CGC,R;ATG,C,ATG,M;ATT,C,"
               "ATT,I;GCG,C,GCG,A;ACC,C,ACC,T;ATT,C,ATT,I;ATT,C,ATT,I;ACC,C,"
               "ACC,T;ACC,C,ACC,T;ACC,C,ACC,T;ATT,C,ATT,I;ACC,C,ACC,T;ACC,C,"
               "ACC,T;CTG,C,CTG,L;GGC,C,GGC,G;GCG,C,GCG,A;,T,,");
        }
    }

    EQ(server_close(), DCP_OK);

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

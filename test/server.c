#include "hope/hope.h"
#include "protein_db_examples.h"
#include "rc.h"
#include "standard_db_examples.h"

void test_srv_setup(void);
void test_srv_reopen(void);
void test_srv_std_db(void);
void test_srv_submit_job(void);
void test_srv_submit_and_fetch_job(unsigned num_threads);

int main(void)
{
    /* test_srv_setup(); */
    /* test_srv_reopen(); */
    /* test_srv_std_db(); */
    /* test_srv_submit_job(); */
    test_srv_submit_and_fetch_job(1);
    /* test_srv_submit_and_fetch_job(4); */
    return hope_status();
}

void test_srv_setup(void)
{
    remove(TMPDIR "/setup.sqlite3");
    EQ(server_open(TMPDIR "/setup.sqlite3", 1), DCP_DONE);
    EQ(server_close(), DCP_DONE);
}

void test_srv_reopen(void)
{
    remove(TMPDIR "/reopen.sqlite3");

    EQ(server_open(TMPDIR "/reopen.sqlite3", 1), DCP_DONE);
    EQ(server_close(), DCP_DONE);

    EQ(server_open(TMPDIR "/reopen.sqlite3", 1), DCP_DONE);
    EQ(server_close(), DCP_DONE);
}

void test_srv_std_db(void)
{
    char const db_path[] = TMPDIR "/std_db.sqlite3";
    char const ex_path[] = TMPDIR "/std_example1.dcp";

    remove(db_path);

    EQ(server_open(db_path, 1), DCP_DONE);

    std_db_examples_new_ex1(ex_path);
    int64_t db_id = 0;
    EQ(server_add_db("std_example1", ex_path, &db_id), DCP_DONE);

    EQ(server_close(), DCP_DONE);
}

void test_srv_submit_job(void)
{
    char const db_path[] = TMPDIR "/submit_job.sqlite3";
    char const ex_path[] = TMPDIR "/std_example1.dcp";
    remove(db_path);

    EQ(server_open(db_path, 1), DCP_DONE);

    std_db_examples_new_ex1(ex_path);
    int64_t db_id = 0;
    EQ(server_add_db("std_example1", ex_path, &db_id), DCP_DONE);
    EQ(db_id, 1);

    struct dcp_job job = {0};
    dcp_job_init(&job, db_id);
    struct dcp_seq seq[2] = {0};
    dcp_seq_init(seq + 0, "seq0", imm_str(imm_example1_seq));
    dcp_seq_init(seq + 1, "seq1", imm_str(imm_example2_seq));
    dcp_job_add_seq(&job, seq + 0);
    dcp_job_add_seq(&job, seq + 1);

    EQ(server_submit_job(&job), DCP_DONE);
    EQ(job.id, 1);

    EQ(server_close(), DCP_DONE);
}

void test_srv_submit_and_fetch_job(unsigned num_threads)
{
    char const db_path[] = TMPDIR "/submit_and_fetch_job.sqlite3";
    char const ex_path[] = TMPDIR "/protein_example1.dcp";
    remove(db_path);

    EQ(server_open(db_path, num_threads), DCP_DONE);

    protein_db_examples_new_ex1(ex_path);
    int64_t db_id = 0;
    EQ(server_add_db("protein_example1", ex_path, &db_id), DCP_DONE);
    EQ(db_id, 1);

    struct dcp_job job = {0};
    dcp_job_init(&job, db_id);
    struct dcp_seq seq[2] = {0};
    dcp_seq_init(seq + 0, "seq0", imm_str(imm_example1_seq));
    dcp_seq_init(seq + 1, "seq1", imm_str(imm_example2_seq));
    dcp_job_add_seq(&job, seq + 0);
    dcp_job_add_seq(&job, seq + 1);

    EQ(server_submit_job(&job), DCP_DONE);
    EQ(job.id, 1);

    enum dcp_job_state state = 0;
    EQ(server_job_state(job.id, &state), DCP_DONE);
    EQ(state, DCP_JOB_PEND);

    EQ(server_job_state(2, &state), DCP_NOTFOUND);

    EQ(server_run(true), DCP_DONE);

    int64_t prod_id = 0;
    EQ(server_next_prod(1, &prod_id), DCP_NEXT);
    EQ(prod_id, 1);

    struct dcp_prod const *p = server_get_prod();

    EQ(p->id, 1);
    EQ(p->seq_id, 2);
    EQ(p->match_id, 1);
    EQ(p->prof_name, "ACC0");
    EQ(p->abc_name, "dna_iupac");
    CLOSE(p->loglik, -2720.38134765625);
    CLOSE(p->null_loglik, -3163.185302734375);
    EQ(p->model, "pro");
    EQ(p->version, "0.0.4");

    extern char const prod1_match_data[];
    EQ(p->match_data, prod1_match_data);

    EQ(server_next_prod(1, &prod_id), DCP_NEXT);
    EQ(prod_id, 2);

    EQ(p->id, 2);
    EQ(p->seq_id, 2);
    EQ(p->match_id, 2);
    EQ(p->prof_name, "ACC1");
    EQ(p->abc_name, "dna_iupac");
    CLOSE(p->loglik, -2854.53369140625);
    CLOSE(p->null_loglik, -3094.66308593750);
    EQ(p->model, "pro");
    EQ(p->version, "0.0.4");

    EQ(server_next_prod(1, &prod_id), DCP_DONE);

    EQ(server_close(), DCP_DONE);
}

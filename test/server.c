#include "server.h"
#include "array.h"
#include "hope/hope.h"
#include "job.h"
#include "protein_db_examples.h"
#include "standard_db_examples.h"

void test_server_setup(unsigned num_threads);
void test_server_reopen(void);
void test_server_standard_db(void);
void test_server_submit_job(void);
void test_server_submit_and_fetch_job(unsigned num_threads);

int main(void)
{
    test_server_setup(1);
    test_server_setup(2);
    test_server_reopen();
    test_server_standard_db();
    /* test_server_submit_job(); */
    /* test_server_submit_and_fetch_job(1); */
    /* test_server_submit_and_fetch_job(4); */
    return hope_status();
}

void test_server_setup(unsigned num_threads)
{
    remove(TMPDIR "/setup.sched");
    EQ(server_open(TMPDIR "/setup.sched", num_threads), RC_DONE);
    EQ(server_close(), RC_DONE);
}

void test_server_reopen(void)
{
    remove(TMPDIR "/reopen.sched");

    EQ(server_open(TMPDIR "/reopen.sched", 1), RC_DONE);
    EQ(server_close(), RC_DONE);

    EQ(server_open(TMPDIR "/reopen.sched", 1), RC_DONE);
    EQ(server_close(), RC_DONE);
}

void test_server_standard_db(void)
{
    char const db_path[] = TMPDIR "/standard_db.sched";
    char const ex_path[] = TMPDIR "/standard_example1.dcp";

    remove(db_path);

    EQ(server_open(db_path, 1), RC_DONE);

    standard_db_examples_new_ex1(ex_path);
    int64_t db_id = 0;
    EQ(server_add_db("standard_example1", ex_path, &db_id), RC_DONE);

    EQ(server_close(), RC_DONE);
}

void test_server_submit_job(void)
{
    char const db_path[] = TMPDIR "/submit_job.sched";
    char const ex_path[] = TMPDIR "/standard_example1.dcp";
    remove(db_path);

    EQ(server_open(db_path, 1), RC_DONE);

    standard_db_examples_new_ex1(ex_path);
    int64_t db_id = 0;
    EQ(server_add_db("standard_example1", ex_path, &db_id), RC_DONE);
    EQ(db_id, 1);

    struct job job = {0};
    job_init(&job, db_id);
    struct seq seq[2] = {0};
    seq_init(seq + 0, "seq0", imm_str(imm_example1_seq));
    seq_init(seq + 1, "seq1", imm_str(imm_example2_seq));
    job_add_seq(&job, seq + 0);
    job_add_seq(&job, seq + 1);

    EQ(server_submit_job(&job), RC_DONE);
    EQ(job.id, 1);

    EQ(server_close(), RC_DONE);
}

void test_server_submit_and_fetch_job(unsigned num_threads)
{
    char const db_path[] = TMPDIR "/submit_and_fetch_job.sched";
    char const ex_path[] = TMPDIR "/protein_example1.dcp";
    remove(db_path);

    EQ(server_open(db_path, num_threads), RC_DONE);

    protein_db_examples_new_ex1(ex_path);
    int64_t db_id = 0;
    EQ(server_add_db("protein_example1", ex_path, &db_id), RC_DONE);
    EQ(db_id, 1);

    struct job job = {0};
    job_init(&job, db_id);
    struct seq seq[2] = {0};
    seq_init(seq + 0, "seq0", imm_str(imm_example1_seq));
    seq_init(seq + 1, "seq1", imm_str(imm_example2_seq));
    job_add_seq(&job, seq + 0);
    job_add_seq(&job, seq + 1);

    EQ(server_submit_job(&job), RC_DONE);
    EQ(job.id, 1);

    enum job_state state = 0;
    EQ(server_job_state(job.id, &state), RC_DONE);
    EQ(state, JOB_PEND);

    EQ(server_job_state(2, &state), RC_NOTFOUND);

    EQ(server_run(true), RC_DONE);

    int64_t prod_id = 0;
    EQ(server_next_prod(1, &prod_id), RC_NEXT);
    EQ(prod_id, 1);

    struct prod const *p = server_get_prod();

    EQ(p->id, 1);
    EQ(p->seq_id, 2);
    EQ(p->match_id, 1);
    EQ(p->prof_name, "ACC0");
    EQ(p->abc_name, "dna_iupac");
    CLOSE(p->loglik, -2720.38134765625);
    CLOSE(p->null_loglik, -3163.185302734375);
    EQ(p->prof_typeid, "protein");
    EQ(p->version, "0.0.4");

    extern char const prod1_match_data[];
    for (unsigned i = 0; i < array_size(p->match); ++i)
    {
        EQ(array_data(p->match)[i], prod1_match_data[i]);
    }

    EQ(server_next_prod(1, &prod_id), RC_NEXT);
    EQ(prod_id, 2);

    EQ(p->id, 2);
    EQ(p->seq_id, 2);
    EQ(p->match_id, 2);
    EQ(p->prof_name, "ACC1");
    EQ(p->abc_name, "dna_iupac");
    CLOSE(p->loglik, -2854.53369140625);
    CLOSE(p->null_loglik, -3094.66308593750);
    EQ(p->prof_typeid, "protein");
    EQ(p->version, "0.0.4");

    EQ(server_next_prod(1, &prod_id), RC_DONE);

    EQ(server_close(), RC_DONE);
}

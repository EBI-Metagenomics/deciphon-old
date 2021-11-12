#include "dcp/dcp.h"
#include "hope/hope.h"
#include "pro_db_examples.h"
#include "std_db_examples.h"

void test_srv_setup(void);
void test_srv_reopen(void);
void test_srv_std_db(void);
void test_srv_submit_job(void);
void test_srv_submit_and_fetch_job(void);

int main(void)
{
    test_srv_setup();
    test_srv_reopen();
    test_srv_std_db();
    test_srv_submit_job();
    test_srv_submit_and_fetch_job();
    return hope_status();
}

void test_srv_setup(void)
{
    remove(TMPDIR "/setup.sqlite3");
    struct dcp_srv *srv = dcp_srv_open(TMPDIR "/setup.sqlite3");
    NOTNULL(srv);
    EQ(dcp_srv_close(srv), DCP_DONE);
}

void test_srv_reopen(void)
{
    remove(TMPDIR "/reopen.sqlite3");

    struct dcp_srv *srv = dcp_srv_open(TMPDIR "/reopen.sqlite3");
    NOTNULL(srv);
    EQ(dcp_srv_close(srv), DCP_DONE);

    srv = dcp_srv_open(TMPDIR "/reopen.sqlite3");
    NOTNULL(srv);
    EQ(dcp_srv_close(srv), DCP_DONE);
}

void test_srv_std_db(void)
{
    char const db_path[] = TMPDIR "/std_db.sqlite3";
    char const ex_path[] = TMPDIR "/std_example1.dcp";

    remove(db_path);

    struct dcp_srv *srv = dcp_srv_open(db_path);
    NOTNULL(srv);

    std_db_examples_new_ex1(ex_path);
    int64_t db_id = 0;
    EQ(dcp_srv_add_db(srv, "std_example1", ex_path, &db_id), DCP_DONE);

    EQ(dcp_srv_close(srv), DCP_DONE);
}

void test_srv_submit_job(void)
{
    char const db_path[] = TMPDIR "/submit_job.sqlite3";
    char const ex_path[] = TMPDIR "/std_example1.dcp";
    remove(db_path);

    struct dcp_srv *srv = dcp_srv_open(db_path);
    NOTNULL(srv);

    std_db_examples_new_ex1(ex_path);
    int64_t db_id = 0;
    EQ(dcp_srv_add_db(srv, "std_example1", ex_path, &db_id), DCP_DONE);
    EQ(db_id, 1);

    struct dcp_job job = {0};
    dcp_job_init(&job, db_id);
    struct dcp_seq seq[2] = {0};
    dcp_seq_init(seq + 0, "seq0", imm_str(imm_example1_seq).data);
    dcp_seq_init(seq + 1, "seq1", imm_str(imm_example2_seq).data);
    dcp_job_add_seq(&job, seq + 0);
    dcp_job_add_seq(&job, seq + 1);

    EQ(dcp_srv_submit_job(srv, &job), DCP_DONE);
    EQ(job.id, 1);

    EQ(dcp_srv_close(srv), DCP_DONE);
}

void test_srv_submit_and_fetch_job(void)
{
    char const db_path[] = TMPDIR "/submit_and_fetch_job.sqlite3";
    char const ex_path[] = TMPDIR "/pro_example1.dcp";
    remove(db_path);

    struct dcp_srv *srv = dcp_srv_open(db_path);
    NOTNULL(srv);

    pro_db_examples_new_ex1(ex_path);
    int64_t db_id = 0;
    EQ(dcp_srv_add_db(srv, "pro_example1", ex_path, &db_id), DCP_DONE);
    EQ(db_id, 1);

    struct dcp_job job = {0};
    dcp_job_init(&job, db_id);
    struct dcp_seq seq[2] = {0};
    dcp_seq_init(seq + 0, "seq0", imm_str(imm_example1_seq).data);
    dcp_seq_init(seq + 1, "seq1", imm_str(imm_example2_seq).data);
    dcp_job_add_seq(&job, seq + 0);
    dcp_job_add_seq(&job, seq + 1);

    EQ(dcp_srv_submit_job(srv, &job), DCP_DONE);
    EQ(job.id, 1);

    enum dcp_job_state state = 0;
    EQ(dcp_srv_job_state(srv, job.id, &state), DCP_DONE);
    EQ(state, DCP_JOB_PEND);

    EQ(dcp_srv_job_state(srv, 2, &state), DCP_NOTFOUND);

    EQ(dcp_srv_run(srv, true), DCP_NEXT);
    EQ(dcp_srv_run(srv, true), DCP_DONE);

    int64_t prod_id = 0;
    EQ(dcp_srv_next_prod(srv, 1, &prod_id), DCP_NEXT);
    EQ(prod_id, 1);

    EQ(dcp_srv_next_prod(srv, 1, &prod_id), DCP_NEXT);
    EQ(prod_id, 2);

    EQ(dcp_srv_next_prod(srv, 1, &prod_id), DCP_DONE);

    EQ(dcp_srv_close(srv), DCP_DONE);
}

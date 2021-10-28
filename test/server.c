#include "dcp/dcp.h"
#include "hope/hope.h"
#include "std_db_examples.h"

void test_server_setup(void);
void test_server_reopen(void);
void test_server_std_db(void);
void test_server_submit_job(void);
void test_server_submit_and_fetch_job(void);

int main(void)
{
    test_server_setup();
    test_server_reopen();
    test_server_std_db();
    test_server_submit_job();
    test_server_submit_and_fetch_job();
    return hope_status();
}

void test_server_setup(void)
{
    remove(TMPDIR "/setup.sqlite3");
    struct dcp_server *srv = dcp_server_open(TMPDIR "/setup.sqlite3");
    NOTNULL(srv);
    EQ(dcp_server_close(srv), DCP_SUCCESS);
}

void test_server_reopen(void)
{
    remove(TMPDIR "/reopen.sqlite3");

    struct dcp_server *srv = dcp_server_open(TMPDIR "/reopen.sqlite3");
    NOTNULL(srv);
    EQ(dcp_server_close(srv), DCP_SUCCESS);

    srv = dcp_server_open(TMPDIR "/reopen.sqlite3");
    NOTNULL(srv);
    EQ(dcp_server_close(srv), DCP_SUCCESS);
}

void test_server_std_db(void)
{
    remove(TMPDIR "/std_db.sqlite3");

    struct dcp_server *srv = dcp_server_open(TMPDIR "/std_db.sqlite3");
    NOTNULL(srv);

    std_db_examples_new_ex1(TMPDIR "/example1.dcp");
    uint64_t db_id = 0;
    EQ(dcp_server_add_db(srv, TMPDIR "/example1.dcp", &db_id), DCP_SUCCESS);

    EQ(dcp_server_close(srv), DCP_SUCCESS);
}

void test_server_submit_job(void)
{
    remove(TMPDIR "/submit_job.sqlite3");

    struct dcp_server *srv = dcp_server_open(TMPDIR "/submit_job.sqlite3");
    NOTNULL(srv);

    std_db_examples_new_ex1(TMPDIR "/example1.dcp");
    uint64_t db_id = 0;
    EQ(dcp_server_add_db(srv, TMPDIR "/example1.dcp", &db_id), DCP_SUCCESS);
    EQ(db_id, 1);

    struct dcp_job job;
    dcp_job_init(&job, true, false);
    struct dcp_seq seq[2] = {0};
    dcp_seq_init(seq + 0, imm_str(imm_example1_seq).data);
    dcp_seq_init(seq + 1, imm_str(imm_example2_seq).data);
    dcp_job_add_seq(&job, seq + 0);
    dcp_job_add_seq(&job, seq + 1);

    uint64_t job_id = 0;
    EQ(dcp_server_submit_job(srv, &job, db_id, &job_id), DCP_SUCCESS);
    EQ(job_id, 1);

    EQ(dcp_server_close(srv), DCP_SUCCESS);
}

void test_server_submit_and_fetch_job(void)
{
    remove(TMPDIR "/submit_and_fetch_job.sqlite3");

    struct dcp_server *srv =
        dcp_server_open(TMPDIR "/submit_and_fetch_job.sqlite3");
    NOTNULL(srv);

    std_db_examples_new_ex1(TMPDIR "/example1.dcp");
    uint64_t db_id = 0;
    EQ(dcp_server_add_db(srv, TMPDIR "/example1.dcp", &db_id), DCP_SUCCESS);
    EQ(db_id, 1);

    struct dcp_job job;
    dcp_job_init(&job, true, false);
    struct dcp_seq seq[2] = {0};
    dcp_seq_init(seq + 0, imm_str(imm_example1_seq).data);
    dcp_seq_init(seq + 1, imm_str(imm_example2_seq).data);
    dcp_job_add_seq(&job, seq + 0);
    dcp_job_add_seq(&job, seq + 1);

    uint64_t job_id = 0;
    EQ(dcp_server_submit_job(srv, &job, db_id, &job_id), DCP_SUCCESS);
    EQ(job_id, 1);

    enum dcp_job_state state = 0;
    EQ(dcp_server_job_state(srv, job_id, &state), DCP_SUCCESS);
    EQ(state, DCP_JOB_PEND);

    EQ(dcp_server_job_state(srv, 2, &state), DCP_NOTFOUND);

    EQ(dcp_server_close(srv), DCP_SUCCESS);
}

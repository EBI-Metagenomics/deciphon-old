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

    int64_t seq_id = 0;
    int64_t match_id = 0;
    char prof_name[DCP_PROF_NAME_SIZE] = {0};
    char abc_name[DCP_ABC_NAME_SIZE] = {0};
    double loglik = 0;
    double null_loglik = 0;
    char model[DCP_MODEL_SIZE] = {0};
    char version[DCP_VERSION_SIZE] = {0};
    char match_data[DCP_MATCH_DATA_SIZE] = {0};

    dcp_srv_prod_seq_id(srv, prod_id, &seq_id);
    dcp_srv_prod_match_id(srv, prod_id, &match_id);
    dcp_srv_prod_prof_name(srv, prod_id, prof_name);
    dcp_srv_prod_abc_name(srv, prod_id, abc_name);
    dcp_srv_prod_loglik(srv, prod_id, &loglik);
    dcp_srv_prod_null_loglik(srv, prod_id, &null_loglik);
    dcp_srv_prod_model(srv, prod_id, model);
    dcp_srv_prod_version(srv, prod_id, version);
    dcp_srv_prod_match_data(srv, prod_id, match_data);

    EQ(prod_id,1);
    EQ(seq_id,2);
    EQ(match_id,1);
    EQ(prof_name,"ACC0");
    EQ(abc_name,"dna_iupac");
    CLOSE(loglik, -2720.38134765625);
    CLOSE(null_loglik, -3163.185302734375);
    EQ(model, "pro");
    EQ(version,"0.0.4");
    extern char const prod1_match_data[];
    EQ(match_data, prod1_match_data);


    EQ(dcp_srv_next_prod(srv, 1, &prod_id), DCP_NEXT);
    EQ(prod_id, 2);

    dcp_srv_prod_seq_id(srv, prod_id, &seq_id);
    dcp_srv_prod_match_id(srv, prod_id, &match_id);
    dcp_srv_prod_prof_name(srv, prod_id, prof_name);
    dcp_srv_prod_abc_name(srv, prod_id, abc_name);
    dcp_srv_prod_loglik(srv, prod_id, &loglik);
    dcp_srv_prod_null_loglik(srv, prod_id, &null_loglik);
    dcp_srv_prod_model(srv, prod_id, model);
    dcp_srv_prod_version(srv, prod_id, version);
    dcp_srv_prod_match_data(srv, prod_id, match_data);

    EQ(dcp_srv_next_prod(srv, 1, &prod_id), DCP_DONE);

    EQ(dcp_srv_close(srv), DCP_DONE);
}

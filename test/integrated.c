#include "hope/hope.h"

void test_sched_submit_product(void);
void test_sched_submit_and_fetch_product(void);

int main(void)
{
    return hope_status();
}

void test_sched_submit_product(void)
{
    char const sched_path[] = TMPDIR "/submit_product.sched";
    char const db_path[] = TMPDIR "/submit_product.dcp";

    remove(sched_path);
    create_file1(db_path);

    EQ(sched_setup(sched_path), DCP_OK);
    EQ(sched_open(), DCP_OK);

    int64_t db_id = 0;
    EQ(sched_add_db(db_path, &db_id), DCP_OK);
    EQ(db_id, 1);

    sched_job_init(&job, db_id, true, false);
    EQ(sched_begin_job_submission(&job), DCP_OK);
    sched_add_seq(&job, "seq0", "ACAAGCAG");
    sched_add_seq(&job, "seq1", "ACTTGCCG");
    EQ(sched_end_job_submission(&job), DCP_OK);

    sched_job_init(&job, db_id, true, true);
    EQ(sched_begin_job_submission(&job), DCP_OK);
    sched_add_seq(&job, "seq0_2", "XXGG");
    sched_add_seq(&job, "seq1_2", "YXYX");
    EQ(sched_end_job_submission(&job), DCP_OK);

    EQ(sched_next_pending_job(&job), DCP_OK);

    EQ(sched_begin_prod_submission(1), DCP_OK);

    prod.id = 0;
    prod.job_id = job.id;
    prod.seq_id = 1;
    strcpy(prod.profile_name, "ACC0");
    strcpy(prod.abc_name, "dna");
    prod.alt_loglik = -2720.381;
    prod.null_loglik = -3163.185;
    strcpy(prod.profile_typeid, "protein");
    strcpy(prod.version, "0.0.4");
    strcpy(prod.match, "A,B,C");

    struct match match0 = {"state0", "GAC"};
    struct match match1 = {"state1", "GGC"};

    EQ(sched_prod_write_begin(&prod, 0), DCP_OK);
    EQ(sched_prod_write_match(write_match_cb, &match0, 0), DCP_OK);
    EQ(sched_prod_write_match_sep(0), DCP_OK);
    EQ(sched_prod_write_match(write_match_cb, &match1, 0), DCP_OK);
    EQ(sched_prod_write_end(0), DCP_OK);

    prod.job_id = job.id;
    prod.seq_id = 2;

    strcpy(prod.profile_name, "ACC1");
    strcpy(prod.abc_name, "dna");

    prod.alt_loglik = -1111.;
    prod.null_loglik = -2222.;

    EQ(sched_prod_write_begin(&prod, 0), DCP_OK);
    EQ(sched_prod_write_match(write_match_cb, &match0, 0), DCP_OK);
    EQ(sched_prod_write_match_sep(0), DCP_OK);
    EQ(sched_prod_write_match(write_match_cb, &match1, 0), DCP_OK);
    EQ(sched_prod_write_end(0), DCP_OK);

    EQ(sched_end_prod_submission(), DCP_OK);

    EQ(sched_close(), DCP_OK);
}

void test_sched_submit_and_fetch_product(void)
{
    char const sched_path[] = TMPDIR "/submit_and_fetch_product.sched";
    char const db_path[] = TMPDIR "/submit_and_fetch_product.dcp";

    remove(sched_path);
    create_file1(db_path);

    EQ(sched_setup(sched_path), DCP_OK);
    EQ(sched_open(), DCP_OK);

    int64_t db_id = 0;
    EQ(sched_add_db(db_path, &db_id), DCP_OK);
    EQ(db_id, 1);

    sched_job_init(&job, db_id, true, false);
    EQ(sched_begin_job_submission(&job), DCP_OK);
    sched_add_seq(&job, "seq0", "ACAAGCAG");
    sched_add_seq(&job, "seq1", "ACTTGCCG");
    EQ(sched_end_job_submission(&job), DCP_OK);

    sched_job_init(&job, db_id, true, true);
    EQ(sched_begin_job_submission(&job), DCP_OK);
    sched_add_seq(&job, "seq0_2", "XXGG");
    sched_add_seq(&job, "seq1_2", "YXYX");
    EQ(sched_end_job_submission(&job), DCP_OK);

    EQ(sched_next_pending_job(&job), DCP_OK);

    EQ(sched_begin_job_submission(&job), DCP_OK);
    EQ(sched_rollback_job_submission(&job), DCP_OK);

    EQ(sched_begin_prod_submission(1), DCP_OK);

    prod.id = 0;
    prod.job_id = job.id;
    prod.seq_id = 1;
    strcpy(prod.profile_name, "ACC0");
    strcpy(prod.abc_name, "dna");
    prod.alt_loglik = -2720.381;
    prod.null_loglik = -3163.185;
    strcpy(prod.profile_typeid, "protein");
    strcpy(prod.version, "0.0.4");

    struct match match0 = {"state0", "GAC"};
    struct match match1 = {"state1", "GGC"};

    EQ(sched_prod_write_begin(&prod, 0), DCP_OK);
    EQ(sched_prod_write_match(write_match_cb, &match0, 0), DCP_OK);
    EQ(sched_prod_write_match_sep(0), DCP_OK);
    EQ(sched_prod_write_match(write_match_cb, &match1, 0), DCP_OK);
    EQ(sched_prod_write_end(0), DCP_OK);

    prod.job_id = job.id;
    prod.seq_id = 2;

    strcpy(prod.profile_name, "ACC1");
    strcpy(prod.abc_name, "dna");

    prod.alt_loglik = -1111.;
    prod.null_loglik = -2222.;

    EQ(sched_prod_write_begin(&prod, 0), DCP_OK);
    EQ(sched_prod_write_match(write_match_cb, &match0, 0), DCP_OK);
    EQ(sched_prod_write_match_sep(0), DCP_OK);
    EQ(sched_prod_write_match(write_match_cb, &match1, 0), DCP_OK);
    EQ(sched_prod_write_end(0), DCP_OK);

    EQ(sched_end_prod_submission(), DCP_OK);

    enum sched_job_state state = 0;

    EQ(sched_set_job_done(1), DCP_OK);
    EQ(sched_job_state(1, &state), DCP_OK);
    EQ(state, SCHED_JOB_DONE);

    EQ(sched_set_job_fail(2, "error msg"), DCP_OK);
    EQ(sched_job_state(2, &state), DCP_OK);
    EQ(state, SCHED_JOB_FAIL);

    sched_prod_init(&prod, 1);
    EQ(sched_prod_next(&prod), RC_NEXT);
    EQ(prod.id, 1);
    EQ(prod.job_id, 1);
    EQ(prod.match, "state0,GAC;state1,GGC");

    EQ(sched_close(), DCP_OK);
}

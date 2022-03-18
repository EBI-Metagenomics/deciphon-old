#include "deciphon/server/sched_api.h"
#include "deciphon/db/db.h"
#include "deciphon/server/server.h"
#include "hope/hope.h"
#include "imm/imm.h"

void test_sched_api_open_close(void);
void test_sched_api_post_db(void);
void test_sched_api_get_db(void);
void test_sched_api_post_testing_data(void);
void test_sched_api_next_pend_job(void);
void test_sched_api_next_job_seq(void);

int main(void)
{
    test_sched_api_open_close();
    test_sched_api_post_db();
    test_sched_api_get_db();
    test_sched_api_post_testing_data();
    test_sched_api_next_pend_job();
    test_sched_api_next_job_seq();
    return hope_status();
}

void test_sched_api_open_close(void)
{
    EQ(sched_api_init(SCHED_API_URL), RC_OK);
    sched_api_cleanup();
}

void test_sched_api_post_db(void)
{
    EQ(sched_api_init(SCHED_API_URL), RC_OK);
    EQ(sched_api_wipe(), RC_OK);

    struct sched_db db = {0};
    struct sched_api_error error = {0};

    strcpy(db.filename, "minifam.dcp");
    EQ(sched_api_add_db(&db, &error), RC_OK);
    EQ(db.id, 1);
    EQ(db.xxh3_64, -3907098992699871052);
    EQ(db.filename, "minifam.dcp");
    EQ(error.rc, SCHED_OK);
    EQ(error.msg, "");

    db.id = 1;
    db.xxh3_64 = -3907098992699871052L;
    strcpy(db.filename, "minifam.dcp");
    EQ(sched_api_add_db(&db, &error), RC_OK);
    EQ(error.rc, SCHED_EINVAL);
    EQ(error.msg, "database already exists");

    sched_api_cleanup();
}

void test_sched_api_get_db(void)
{
    EQ(sched_api_init(SCHED_API_URL), RC_OK);
    EQ(sched_api_wipe(), RC_OK);

    struct sched_db db = {0};
    struct sched_api_error error = {0};

    db.id = 1;
    db.xxh3_64 = 0;
    db.filename[0] = 0;
    EQ(sched_api_get_db(&db, &error), RC_OK);
    EQ(error.rc, SCHED_EINVAL);
    EQ(error.msg, "database not found");

    strcpy(db.filename, "minifam.dcp");
    EQ(sched_api_add_db(&db, &error), RC_OK);
    EQ(db.id, 1);
    EQ(db.xxh3_64, -3907098992699871052);
    EQ(db.filename, "minifam.dcp");
    EQ(error.rc, SCHED_OK);
    EQ(error.msg, "");

    db.id = 1;
    db.xxh3_64 = 0;
    db.filename[0] = 0;
    EQ(sched_api_get_db(&db, &error), RC_OK);
    EQ(db.id, 1);
    EQ(db.xxh3_64, -3907098992699871052);
    EQ(db.filename, "minifam.dcp");
    EQ(error.rc, SCHED_OK);
    EQ(error.msg, "");

    sched_api_cleanup();
}

void test_sched_api_post_testing_data(void)
{
    EQ(sched_api_init(SCHED_API_URL), RC_OK);
    EQ(sched_api_wipe(), RC_OK);

    struct sched_api_error error = {0};
    EQ(sched_api_post_testing_data(&error), RC_OK);

    EQ(error.rc, SCHED_OK);
    EQ(error.msg, "");

    sched_api_cleanup();
}

void test_sched_api_next_pend_job(void)
{
    EQ(sched_api_init(SCHED_API_URL), RC_OK);
    EQ(sched_api_wipe(), RC_OK);

    struct sched_job job = {0};
    struct sched_api_error error = {0};

    EQ(sched_api_next_pend_job(&job, &error), RC_END);
    EQ(error.rc, SCHED_OK);
    EQ(error.msg, "");
    EQ(job.id, 0);

    EQ(sched_api_post_testing_data(&error), RC_OK);
    EQ(error.rc, SCHED_OK);
    EQ(error.msg, "");

    EQ(sched_api_next_pend_job(&job, &error), RC_OK);
    EQ(error.rc, SCHED_OK);
    EQ(error.msg, "");
    EQ(job.id, 1);
    EQ(job.db_id, 1);
    EQ(job.multi_hits, 1);
    EQ(job.hmmer3_compat, 0);
    EQ(job.state, "pend");
    EQ(job.error, "");
    COND(job.submission > 1646972352);
    EQ(job.exec_started, 0);
    EQ(job.exec_ended, 0);

    sched_api_cleanup();
}

static struct sched_seq seq = {0};

void test_sched_api_next_job_seq(void)
{
    EQ(sched_api_init(SCHED_API_URL), RC_OK);
    EQ(sched_api_wipe(), RC_OK);

    struct sched_job job = {0};
    struct sched_api_error error = {0};

    EQ(sched_api_next_pend_job(&job, &error), RC_END);
    EQ(error.rc, SCHED_OK);
    EQ(error.msg, "");
    EQ(job.id, 0);

    EQ(sched_api_post_testing_data(&error), RC_OK);
    EQ(error.rc, SCHED_OK);
    EQ(error.msg, "");

    EQ(sched_api_next_pend_job(&job, &error), RC_OK);
    EQ(error.rc, SCHED_OK);
    EQ(error.msg, "");
    EQ(job.id, 1);
    EQ(job.db_id, 1);
    EQ(job.multi_hits, 1);
    EQ(job.hmmer3_compat, 0);
    EQ(job.state, "pend");
    EQ(job.error, "");
    COND(job.submission > 1646972352);
    EQ(job.exec_started, 0);
    EQ(job.exec_ended, 0);

    seq.id = 0;
    EQ(sched_api_next_job_seq(&job, &seq, &error), RC_OK);

    sched_api_cleanup();
}

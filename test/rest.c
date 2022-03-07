#include "deciphon/db/db.h"
#include "deciphon/server/server.h"
#include "hope/hope.h"
#include "imm/imm.h"

void test_rest_open_close(void);
void test_rest_post_db(void);
void test_rest_get_db(void);
// void test_rest_next_pend_job(void);

int main(void)
{
    test_rest_open_close();
    test_rest_post_db();
    test_rest_get_db();
    // test_rest_next_pend_job();
    return hope_status();
}

void test_rest_open_close(void)
{
    EQ(rest_open(REST_URL_STEM), RC_OK);
    rest_close();
}

void test_rest_post_db(void)
{
    EQ(rest_open(REST_URL_STEM), RC_OK);
    EQ(rest_wipe(), RC_OK);

    struct sched_db db = {0};
    strcpy(db.filename, "minifam.dcp");
    EQ(rest_post_db(&db), RC_OK);

    rest_close();
}

void test_rest_get_db(void)
{
    EQ(rest_open(REST_URL_STEM), RC_OK);
    EQ(rest_wipe(), RC_OK);

    struct sched_db db = {0};
    strcpy(db.filename, "minifam.dcp");
    EQ(rest_post_db(&db), RC_OK);

    db.id = 1;
    db.xxh3_64 = 0;
    db.filename[0] = 0;
    EQ(rest_get_db(&db), RC_OK);

    rest_close();
}

void test_rest_next_pend_job(void)
{
    EQ(rest_open(REST_URL_STEM), RC_OK);
    EQ(rest_wipe(), RC_OK);

    struct sched_job job = {0};
    EQ(rest_next_pend_job(&job), RC_OK);
    EQ(job.id, 1);
    EQ(job.db_id, 1);
    EQ(job.multi_hits, 1);
    EQ(job.hmmer3_compat, 0);
    EQ(job.state, "pend");
    EQ(job.error, "");
    COND(job.submission > 1646384350);
    EQ(job.exec_started, 0);
    EQ(job.exec_ended, 0);

    rest_close();
}

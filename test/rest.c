#include "deciphon/db/db.h"
#include "deciphon/server/server.h"
#include "hope/hope.h"
#include "imm/imm.h"

void test_rest_open_close(void);
void test_rest_next_pend_job(void);
void test_rest_post_db(void);
void test_rest_get_db(void);

int main(void)
{
    test_rest_open_close();
    test_rest_next_pend_job();
    test_rest_post_db();
    test_rest_get_db();
    return hope_status();
}

void test_rest_open_close(void)
{
    EQ(rest_open(REST_URL_STEM), RC_OK);
    rest_close();
}

void test_rest_next_pend_job(void)
{
    EQ(rest_open(REST_URL_STEM), RC_OK);
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

void test_rest_post_db(void)
{
    EQ(rest_open(REST_URL_STEM), RC_OK);
    struct sched_db db = {0};
    strcpy(db.filename, "minifam.dcp");
    EQ(rest_post_db(&db), RC_OK);
    // {"id":1,"xxh3_64":-5972818837115870266,"filename":"minifam.dcp"}HTTP
    // code: 201
    rest_close();
}

void test_rest_get_db(void)
{
    EQ(rest_open(REST_URL_STEM), RC_OK);
    struct sched_db db = {0};
    db.id = 1;
    EQ(rest_get_db(&db), RC_OK);
    // [{"id":1,"xxh3_64":-5972818837115870266,"filename":"minifam.dcp"}]HTTP
    // code: 200
    rest_close();
}

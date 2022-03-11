#include "deciphon/db/db.h"
#include "deciphon/server/server.h"
#include "hope/hope.h"
#include "imm/imm.h"
#include <curl/curl.h>

void test_rest_open_close(void);
void test_rest_post_db(void);
void test_rest_get_db(void);
void test_rest_post_testing_data(void);
void test_rest_next_pend_job(void);

int main(void)
{
    test_rest_open_close();
    test_rest_post_db();
    test_rest_get_db();
    test_rest_post_testing_data();
    test_rest_next_pend_job();
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
    struct rest_error error = {0};

    strcpy(db.filename, "minifam.dcp");
    EQ(rest_post_db(&db, &error), RC_OK);
    EQ(db.id, 1);
    EQ(db.xxh3_64, -3907098992699871052);
    EQ(db.filename, "minifam.dcp");
    EQ(error.rc, RC_OK);
    EQ(error.msg, "");

    db.id = 1;
    db.xxh3_64 = -3907098992699871052L;
    strcpy(db.filename, "minifam.dcp");
    EQ(rest_post_db(&db, &error), RC_OK);
    EQ(error.rc, RC_EINVAL);
    EQ(error.msg, "database already exists");

    rest_close();
}

void test_rest_get_db(void)
{
    EQ(rest_open(REST_URL_STEM), RC_OK);
    EQ(rest_wipe(), RC_OK);

    struct sched_db db = {0};
    struct rest_error error = {0};

    db.id = 1;
    db.xxh3_64 = 0;
    db.filename[0] = 0;
    EQ(rest_get_db(&db, &error), RC_OK);
    EQ(error.rc, RC_EINVAL);
    EQ(error.msg, "database not found");

    strcpy(db.filename, "minifam.dcp");
    EQ(rest_post_db(&db, &error), RC_OK);
    EQ(db.id, 1);
    EQ(db.xxh3_64, -3907098992699871052);
    EQ(db.filename, "minifam.dcp");
    EQ(error.rc, RC_OK);
    EQ(error.msg, "");

    db.id = 1;
    db.xxh3_64 = 0;
    db.filename[0] = 0;
    EQ(rest_get_db(&db, &error), RC_OK);
    EQ(db.id, 1);
    EQ(db.xxh3_64, -3907098992699871052);
    EQ(db.filename, "minifam.dcp");
    EQ(error.rc, RC_OK);
    EQ(error.msg, "");

    rest_close();
}

void test_rest_post_testing_data(void)
{
    EQ(rest_open(REST_URL_STEM), RC_OK);
    EQ(rest_wipe(), RC_OK);

    struct rest_error error = {0};
    EQ(rest_testing_data(&error), RC_OK);

    EQ(error.rc, RC_OK);
    EQ(error.msg, "");

    rest_close();
}

void test_rest_next_pend_job(void)
{
    EQ(rest_open(REST_URL_STEM), RC_OK);
    EQ(rest_wipe(), RC_OK);

    struct sched_job job = {0};
    struct rest_error error = {0};

    EQ(rest_next_pend_job(&job, &error), RC_OK);
    EQ(error.rc, RC_OK);
    EQ(error.msg, "");
    EQ(job.id, 0);

    EQ(rest_testing_data(&error), RC_OK);
    EQ(error.rc, RC_OK);
    EQ(error.msg, "");

    EQ(rest_next_pend_job(&job, &error), RC_OK);
    EQ(error.rc, RC_OK);
    EQ(error.msg, "");
    EQ(job.id, 0);

    rest_close();
}

#include "deciphon/server/sched_api.h"
#include "deciphon/db/db.h"
#include "deciphon/server/sched.h"
#include "deciphon/server/server.h"
#include "deciphon/xfile.h"
#include "hope/hope.h"
#include "imm/imm.h"

static struct sched_hmm hmm = {0};
static struct sched_db db = {0};
static struct sched_job job = {0};
static struct sched_api_error error = {0};

void test_sched_api_open_close(void);
void test_sched_api_no_pend_job(void);

void test_sched_api_upload_hmm(void);
void test_sched_api_get_hmm(void);
void test_sched_api_download_hmm(void);

void test_sched_api_upload_db(void);
void test_sched_api_get_db(void);
void test_sched_api_download_db(void);

// void test_sched_api_post_testing_data(void);
// void test_sched_api_next_pend_job(void);
// void test_sched_api_next_job_seq(void);

int main(void)
{
    test_sched_api_open_close();
    test_sched_api_no_pend_job();

    test_sched_api_upload_hmm();
    test_sched_api_get_hmm();
    test_sched_api_download_hmm();

    test_sched_api_upload_db();
    test_sched_api_get_db();
    test_sched_api_download_db();

    // test_sched_api_post_testing_data();
    // test_sched_api_next_pend_job();
    // test_sched_api_next_job_seq();
    return hope_status();
}

void test_sched_api_open_close(void)
{
    EQ(sched_api_init(SCHED_API_URL), RC_OK);
    sched_api_cleanup();
}

void test_sched_api_no_pend_job(void)
{
    EQ(sched_api_init(SCHED_API_URL), RC_OK);
    EQ(sched_api_wipe(), RC_OK);

    EQ(sched_api_next_pend_job(&job, &error), RC_END);
    EQ(error.rc, SCHED_OK);
    EQ(error.msg, "");
    EQ(job.id, 0);

    sched_api_cleanup();
}

void test_sched_api_upload_hmm(void)
{
    EQ(sched_api_init(SCHED_API_URL), RC_OK);
    EQ(sched_api_wipe(), RC_OK);

    EQ(sched_api_upload_hmm(ASSETS "/PF02545.hmm", &hmm, &error), RC_OK);
    EQ(hmm.id, 1);
    EQ(hmm.xxh3, -7843725841264658444);
    EQ(hmm.filename, "PF02545.hmm");
    EQ(hmm.job_id, 1);
    EQ(error.rc, SCHED_OK);
    EQ(error.msg, "");

    EQ(sched_api_next_pend_job(&job, &error), RC_OK);
    EQ(error.rc, SCHED_OK);
    EQ(error.msg, "");
    EQ(job.id, 1);
    EQ(sched_job_type(&job), SCHED_HMM);

    sched_api_cleanup();
}

void test_sched_api_get_hmm(void)
{
    EQ(sched_api_init(SCHED_API_URL), RC_OK);
    EQ(sched_api_wipe(), RC_OK);

    EQ(sched_api_upload_hmm(ASSETS "/PF02545.hmm", &hmm, &error), RC_OK);
    EQ(hmm.id, 1);
    EQ(hmm.xxh3, -7843725841264658444);
    EQ(hmm.filename, "PF02545.hmm");
    EQ(hmm.job_id, 1);
    EQ(error.rc, SCHED_OK);
    EQ(error.msg, "");

    EQ(sched_api_next_pend_job(&job, &error), RC_OK);
    EQ(error.rc, SCHED_OK);
    EQ(error.msg, "");
    EQ(job.id, 1);
    EQ(sched_job_type(&job), SCHED_HMM);

    EQ(sched_api_get_hmm(hmm.id, &hmm, &error), RC_OK);
    EQ(hmm.id, 1);
    EQ(hmm.xxh3, -7843725841264658444);
    EQ(hmm.filename, "PF02545.hmm");
    EQ(hmm.job_id, 1);

    EQ(sched_api_get_hmm_by_job_id(job.id, &hmm, &error), RC_OK);
    EQ(hmm.id, 1);
    EQ(hmm.xxh3, -7843725841264658444);
    EQ(hmm.filename, "PF02545.hmm");
    EQ(hmm.job_id, 1);

    sched_api_cleanup();
}

void test_sched_api_download_hmm(void)
{
    EQ(sched_api_init(SCHED_API_URL), RC_OK);
    EQ(sched_api_wipe(), RC_OK);

    EQ(sched_api_upload_hmm(ASSETS "/PF02545.hmm", &hmm, &error), RC_OK);
    EQ(hmm.id, 1);
    EQ(hmm.xxh3, -7843725841264658444);
    EQ(hmm.filename, "PF02545.hmm");
    EQ(hmm.job_id, 1);
    EQ(error.rc, SCHED_OK);
    EQ(error.msg, "");

    FILE *fp = fopen(TMPDIR "/db.hmm", "wb");
    EQ(sched_api_download_hmm(hmm.id, fp), RC_OK);
    fclose(fp);

    fp = fopen(TMPDIR "/db.hmm", "rb");
    int64_t hash = 0;
    EQ(xfile_hash(fp, &hash), RC_OK);
    EQ(hmm.xxh3, hash);
    fclose(fp);

    sched_api_cleanup();
}

void test_sched_api_upload_db(void)
{
    EQ(sched_api_init(SCHED_API_URL), RC_OK);
    EQ(sched_api_wipe(), RC_OK);

    EQ(sched_api_upload_hmm(ASSETS "/PF02545.hmm", &hmm, &error), RC_OK);

    EQ(sched_api_set_job_state(hmm.job_id, SCHED_RUN, "", &error), RC_OK);
    EQ(sched_api_set_job_state(hmm.job_id, SCHED_DONE, "", &error), RC_OK);

    EQ(sched_api_upload_db(ASSETS "/PF02545.hmm", &db, &error), RC_EFAIL);

    EQ(sched_api_upload_db(ASSETS "/PF02545.dcp", &db, &error), RC_OK);
    EQ(db.id, 1);
    EQ(db.xxh3, -7843725841264658444);
    EQ(db.filename, "PF02545.dcp");
    EQ(db.hmm_id, hmm.id);
    EQ(error.rc, SCHED_OK);
    EQ(error.msg, "");

    sched_api_cleanup();
}

void test_sched_api_get_db(void)
{
    EQ(sched_api_init(SCHED_API_URL), RC_OK);
    EQ(sched_api_wipe(), RC_OK);

    EQ(sched_api_upload_hmm(ASSETS "/PF02545.hmm", &hmm, &error), RC_OK);

    EQ(sched_api_set_job_state(hmm.job_id, SCHED_RUN, "", &error), RC_OK);
    EQ(sched_api_set_job_state(hmm.job_id, SCHED_DONE, "", &error), RC_OK);

    EQ(sched_api_upload_db(ASSETS "/PF02545.dcp", &db, &error), RC_OK);

    EQ(sched_api_get_db(db.id, &db, &error), RC_OK);
    EQ(db.id, 1);
    EQ(db.xxh3, -7843725841264658444);
    EQ(db.filename, "PF02545.dcp");
    EQ(db.hmm_id, hmm.id);
    EQ(error.rc, SCHED_OK);
    EQ(error.msg, "");

    sched_api_cleanup();
}

void test_sched_api_download_db(void)
{
    EQ(sched_api_init(SCHED_API_URL), RC_OK);
    EQ(sched_api_wipe(), RC_OK);

    EQ(sched_api_upload_hmm(ASSETS "/PF02545.hmm", &hmm, &error), RC_OK);

    EQ(sched_api_set_job_state(hmm.job_id, SCHED_RUN, "", &error), RC_OK);
    EQ(sched_api_set_job_state(hmm.job_id, SCHED_DONE, "", &error), RC_OK);

    EQ(sched_api_upload_db(ASSETS "/PF02545.dcp", &db, &error), RC_OK);

    EQ(sched_api_get_db(db.id, &db, &error), RC_OK);
    EQ(db.id, 1);
    EQ(db.xxh3, -7843725841264658444);
    EQ(db.filename, "PF02545.dcp");
    EQ(db.hmm_id, hmm.id);
    EQ(error.rc, SCHED_OK);
    EQ(error.msg, "");

    FILE *fp = fopen(TMPDIR "/db.dcp", "wb");
    EQ(sched_api_download_db(db.id, fp), RC_OK);
    fclose(fp);

    fp = fopen(TMPDIR "/db.dcp", "rb");
    int64_t hash = 0;
    EQ(xfile_hash(fp, &hash), RC_OK);
    EQ(db.xxh3, hash);
    fclose(fp);

    sched_api_cleanup();
}

#if 0
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
    EQ(job.type, 0);
    EQ(job.state, "pend");
    EQ(job.progress, 0);
    EQ(job.error, "");
    COND(job.submission > 1646972352);
    EQ(job.exec_started, 0);
    EQ(job.exec_ended, 0);

    sched_api_cleanup();
}

// static struct sched_seq seq = {0};

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
    EQ(job.type, 0);
    EQ(job.state, "pend");
    EQ(job.progress, 0);
    EQ(job.error, "");
    COND(job.submission > 1646972352);
    EQ(job.exec_started, 0);
    EQ(job.exec_ended, 0);

    // seq.id = 0;
    // EQ(sched_api_next_job_seq(&job, &seq, &error), RC_OK);

    sched_api_cleanup();
}
#endif

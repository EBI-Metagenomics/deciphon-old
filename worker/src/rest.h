#ifndef REST_H
#define REST_H

#include "common/limits.h"
#include "common/rc.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

struct sched_seq
{
    int64_t id;
    int64_t job_id;
    char name[SEQ_NAME_SIZE];
    char data[SEQ_SIZE];
};

struct rest_job_state
{
    enum rc rc;
    char error[JOB_ERROR_SIZE];
    char state[JOB_STATE_SIZE];
};

struct rest_pend_job
{
    int64_t id;
    int64_t db_id;
    bool multi_hits;
    bool hmmer3_compat;
};

enum sched_job_state
{
    SCHED_JOB_PEND,
    SCHED_JOB_RUN,
    SCHED_JOB_DONE,
    SCHED_JOB_FAIL
};

struct sched_job
{
    int64_t id;

    int64_t db_id;
    int32_t multi_hits;
    int32_t hmmer3_compat;
    char state[JOB_STATE_SIZE];

    char error[JOB_ERROR_SIZE];
    int64_t submission;
    int64_t exec_started;
    int64_t exec_ended;
};

struct sched_db
{
    int64_t id;
    int64_t xxh64;
    char filename[FILENAME_SIZE];
};

struct rest_ret
{
    enum rc rc;
    char error[ERROR_SIZE];
};

extern struct rest_ret rest_ret;
extern struct rest_job_state job_state;

enum rc rest_open(char const *url);
void rest_close(void);
// enum rc rest_job_state(int64_t job_id);
enum rc rest_next_pend_job(struct sched_job *job);
enum rc rest_set_job_state(struct sched_job *job, enum sched_job_state state, char const *error);
// enum rc rest_get_db_filepath(unsigned size, char *filepath, int64_t id);
enum rc rest_get_db(struct sched_db *db);
enum rc rest_next_seq(struct sched_seq *seq);
enum rc rest_submit_prods_file(char const *filepath);

#endif

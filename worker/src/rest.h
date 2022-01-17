#ifndef REST_H
#define REST_H

#include "common/limits.h"
#include "common/rc.h"
#include "work.h"
#include <stdint.h>

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

extern struct rest_job_state job_state;

enum rc rest_job_state(int64_t job_id);
enum rc rest_set_job_fail(int64_t job_id, char const *error);
enum rc rest_set_job_done(int64_t job_id);
enum rc rest_next_pend_job(struct rest_pend_job *job);
enum rc rest_get_db_filepath(unsigned size, char *filepath, int64_t id);
enum rc rest_next_seq(struct sched_seq *seq);

#endif

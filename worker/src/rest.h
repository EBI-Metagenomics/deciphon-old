#ifndef REST_H
#define REST_H

#include "sched/job.h"

struct rest_job_state
{
    enum rc rc;
    char error[JOB_ERROR_SIZE];
    char state[JOB_STATE_SIZE];
};

extern struct rest_job_state job_state;

enum rc rest_job_state(int64_t job_id);

#endif

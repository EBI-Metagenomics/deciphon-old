#ifndef DECIPHON_SERVER_SCHED_H
#define DECIPHON_SERVER_SCHED_H

#include "deciphon/limits.h"
#include <stdint.h>

struct sched_seq
{
    int64_t id;
    int64_t job_id;
    char name[SEQ_NAME_SIZE];
    char data[SEQ_SIZE];
};

struct sched_db
{
    int64_t id;
    int64_t xxh3_64;
    char filename[FILENAME_SIZE];
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

#endif

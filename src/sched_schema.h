#ifndef SCHED_SCHEMA_H
#define SCHED_SCHEMA_H

#include "path_size.h"
#include <stdint.h>

#define SCHED_SCHEMA_STATE_LEN 4
#define SCHED_SCHEMA_SHORT_LEN 15
#define SCHED_SCHEMA_ERROR_LEN 63
#define SCHED_SCHEMA_NAME_LEN 255
#define SCHED_SCHEMA_DATA_LEN 16383

struct sched_job
{
    int64_t id;
    int64_t db_id;
    int32_t multi_hits;
    int32_t hmmer3_compat;
    char state[SCHED_SCHEMA_STATE_LEN + 1];
    char error[SCHED_SCHEMA_ERROR_LEN + 1];
    int64_t submission;
    int64_t exec_started;
    int64_t exec_ended;
};

struct sched_seq
{
    int64_t id;
    int64_t job_id;
    char name[SCHED_SCHEMA_NAME_LEN + 1];
    char data[SCHED_SCHEMA_DATA_LEN + 1];
};

struct sched_prod
{
    int64_t id;
    int64_t job_id;
    int64_t seq_id;
    int64_t match_id;
    char prof_name[SCHED_SCHEMA_NAME_LEN + 1];
    int64_t start_pos;
    int64_t end_pos;
    char abc_id[SCHED_SCHEMA_SHORT_LEN + 1];
    char loglik[SCHED_SCHEMA_SHORT_LEN + 1];
    char null_loglik[SCHED_SCHEMA_SHORT_LEN + 1];
    char model[SCHED_SCHEMA_SHORT_LEN + 1];
    char version[SCHED_SCHEMA_SHORT_LEN + 1];
    char match_data[SCHED_SCHEMA_DATA_LEN + 1];
};

struct sched_db
{
    int64_t id;
    char name[SCHED_SCHEMA_NAME_LEN + 1];
    char filepath[PATH_SIZE];
};

extern unsigned char sched_schema[];

#endif

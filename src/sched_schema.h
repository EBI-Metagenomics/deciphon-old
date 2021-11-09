#ifndef SCHED_SCHEMA_H
#define SCHED_SCHEMA_H

#include "path_size.h"
#include <stdlib.h>

#define SCHED_SCHEMA_STATE_SIZE 5
#define SCHED_SCHEMA_SHORT_SIZE 16
#define SCHED_SCHEMA_ERROR_SIZE 64
#define SCHED_SCHEMA_NAME_SIZE 256
#define SCHED_SCHEMA_DATA_SIZE 16384

struct sched_job
{
    int64_t id;
    int64_t db_id;
    int32_t multi_hits;
    int32_t hmmer3_compat;
    char state[SCHED_SCHEMA_STATE_SIZE];
    char error[SCHED_SCHEMA_ERROR_SIZE];
    int64_t submission;
    int64_t exec_started;
    int64_t exec_ended;
};

struct sched_seq
{
    int64_t id;
    int64_t job_id;
    char name[SCHED_SCHEMA_NAME_SIZE];
    char data[SCHED_SCHEMA_DATA_SIZE];
};

struct sched_prod
{
    int64_t id;
    int64_t job_id;
    int64_t seq_id;
    int64_t match_id;
    char prof_name[SCHED_SCHEMA_NAME_SIZE];
    int64_t start_pos;
    int64_t end_pos;
    char abc_id[SCHED_SCHEMA_SHORT_SIZE];
    char loglik[SCHED_SCHEMA_SHORT_SIZE];
    char null_loglik[SCHED_SCHEMA_SHORT_SIZE];
    char model[SCHED_SCHEMA_SHORT_SIZE];
    char version[SCHED_SCHEMA_SHORT_SIZE];
    char match_data[SCHED_SCHEMA_DATA_SIZE];
};

struct sched_db
{
    int64_t id;
    char name[SCHED_SCHEMA_NAME_SIZE];
    char filepath[PATH_SIZE];
};

extern unsigned char const sched_schema[];
extern size_t const sched_schema_size;

#endif

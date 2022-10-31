#ifndef SCHED_STRUCTS_H
#define SCHED_STRUCTS_H

#include <assert.h>
#include <stdint.h>

enum sched_limits
{
    SCHED_ABC_NAME_SIZE = 16,
    SCHED_FILENAME_SIZE = 128,
    SCHED_JOB_ERROR_SIZE = 256,
    SCHED_JOB_STATE_SIZE = 5,
    SCHED_MATCH_SIZE = 5 * (1024 * 1024),
    SCHED_MAX_NUM_THREADS = 64,
    SCHED_NUM_SEQS_PER_JOB = 512,
    SCHED_PATH_SIZE = 4096,
    SCHED_PROFILE_NAME_SIZE = 64,
    SCHED_PROFILE_TYPEID_SIZE = 16,
    SCHED_SEQ_NAME_SIZE = 256,
    SCHED_SEQ_SIZE = (1024 * 1024),
    SCHED_VERSION_SIZE = 16,
};

static_assert(sizeof(long) >= 8);

struct sched_scan
{
    long id;
    long db_id;

    int multi_hits;
    int hmmer3_compat;

    long job_id;
};

struct sched_hmm
{
    long id;
    long xxh3;
    char filename[SCHED_FILENAME_SIZE];
    long job_id;
};

struct sched_db
{
    long id;
    long xxh3;
    char filename[SCHED_FILENAME_SIZE];
    long hmm_id;
};

struct sched_prod
{
    long id;

    long scan_id;
    long seq_id;

    char profile_name[SCHED_PROFILE_NAME_SIZE];
    char abc_name[SCHED_ABC_NAME_SIZE];

    double alt_loglik;
    double null_loglik;

    char profile_typeid[SCHED_PROFILE_TYPEID_SIZE];
    char version[SCHED_VERSION_SIZE];

    char match[SCHED_MATCH_SIZE];
};

enum sched_job_type
{
    SCHED_SCAN,
    SCHED_HMM
};

enum sched_job_state
{
    SCHED_PEND,
    SCHED_RUN,
    SCHED_DONE,
    SCHED_FAIL
};

struct sched_job
{
    long id;
    int type;

    char state[SCHED_JOB_STATE_SIZE];
    int progress;
    char error[SCHED_JOB_ERROR_SIZE];

    long submission;
    long exec_started;
    long exec_ended;
};

struct sched_seq
{
    long id;
    long scan_id;
    char name[SCHED_SEQ_NAME_SIZE];
    char data[SCHED_SEQ_SIZE];
};

#endif

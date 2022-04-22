#ifndef DECIPHON_SERVER_SCHED_H
#define DECIPHON_SERVER_SCHED_H

#include "deciphon/limits.h"
#include <stdbool.h>
#include <stdint.h>

struct sched_seq
{
    int64_t id;
    int64_t scan_id;
    char name[SEQ_NAME_SIZE];
    char data[SEQ_SIZE];
};

struct sched_hmm
{
    int64_t id;
    int64_t xxh3;
    char filename[FILENAME_SIZE];
    int64_t job_id;
};

struct sched_db
{
    int64_t id;
    int64_t xxh3;
    char filename[FILENAME_SIZE];
    int64_t hmm_id;
};

struct sched_scan
{
    int64_t id;
    int64_t db_id;

    bool multi_hits;
    bool hmmer3_compat;

    int64_t job_id;
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
    int64_t id;
    int type;

    char state[JOB_STATE_SIZE];
    int progress;
    char error[JOB_ERROR_SIZE];

    int64_t submission;
    int64_t exec_started;
    int64_t exec_ended;
};

static inline enum sched_job_type sched_job_type(struct sched_job const *job)
{
    return (enum sched_job_type)job->type;
}

struct xjson;

void sched_db_init(struct sched_db *);
void sched_hmm_init(struct sched_hmm *);
void sched_job_init(struct sched_job *);
void sched_scan_init(struct sched_scan *);
void sched_seq_init(struct sched_seq *);

enum rc sched_db_parse(struct sched_db *, struct xjson *x, unsigned start);
enum rc sched_hmm_parse(struct sched_hmm *, struct xjson *x, unsigned start);
enum rc sched_job_parse(struct sched_job *, struct xjson *x, unsigned start);
enum rc sched_scan_parse(struct sched_scan *, struct xjson *x, unsigned start);
enum rc sched_seq_parse(struct sched_seq *, struct xjson *x, unsigned start);

#endif

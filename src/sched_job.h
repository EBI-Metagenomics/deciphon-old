#ifndef SCHED_JOB_H
#define SCHED_JOB_H

#include "dcp/job_state.h"
#include "dcp/rc.h"
#include "sched_seq.h"
#include "utc.h"
#include <stdbool.h>

#define SCHED_JOB_STATE_ERROR_SIZE 32

struct sched_job
{
    uint64_t id;
    bool multi_hits;
    bool hmmer3_compat;
    uint64_t db_id;
    enum dcp_job_state state;
    char error[SCHED_JOB_STATE_ERROR_SIZE];
    dcp_utc submission;
    dcp_utc exec_started;
    dcp_utc exec_ended;
    struct cco_queue seqs;
};

void sched_job_setup(struct sched_job *job, bool multi_hits, bool hmmer3_compat,
                     uint64_t db_id);

void sched_job_add_seq(struct sched_job *job, struct sched_seq *seq);

#endif

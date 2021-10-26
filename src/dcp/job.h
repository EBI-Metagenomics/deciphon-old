#ifndef DCP_JOB_H
#define DCP_JOB_H

#include "dcp/abc.h"
#include "dcp/export.h"
#include "dcp/id.h"
#include "dcp/rc.h"
#include "dcp/seq.h"
#include "dcp/utc.h"
#include <stdbool.h>

#define DCP_JOB_SID_SIZE 20

#define DCP_JOB_STATE_ERROR_SIZE 32

struct dcp_job;

struct dcp_job_cfg
{
    bool multiple_hits;
    bool hmmer3_compat;
};

enum dcp_job_state
{
    DCP_JOB_STATE_PEND,
    DCP_JOB_STATE_RUN,
    DCP_JOB_STATE_DONE,
    DCP_JOB_STATE_FAIL
};

DCP_API extern struct dcp_job_cfg const dcp_job_cfg_default;

struct dcp_job
{
    dcp_id id;
    char sid[DCP_JOB_SID_SIZE];
    struct dcp_job_cfg cfg;
    struct dcp_abc const *abc;
    /* struct dcp_abc const *db; */
    enum dcp_job_state state;
    char error[DCP_JOB_STATE_ERROR_SIZE];
    dcp_utc submission;
    dcp_utc exec_started;
    dcp_utc exec_ended;
    unsigned contacted;
    dcp_utc last_contacted;
    dcp_id user_id;
    struct cco_queue seqs;
};

static inline void dcp_job_init(struct dcp_job *job, struct dcp_abc const *abc)
{
    job->cfg = dcp_job_cfg_default;
    job->abc = abc;
    cco_queue_init(&job->seqs);
}

static inline void dcp_job_setup(struct dcp_job *job, struct dcp_job_cfg cfg)
{
    job->cfg = cfg;
}

DCP_API enum dcp_rc dcp_job_add(struct dcp_job *job, struct dcp_seq *seq);

#endif

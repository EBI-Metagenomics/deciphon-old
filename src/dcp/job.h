#ifndef DCP_JOB_H
#define DCP_JOB_H

#include "dcp/export.h"
#include "dcp/rc.h"
#include "dcp/seq.h"
#include <stdbool.h>

#define DCP_JOB_SID_SIZE 20

struct dcp_job;
struct imm_abc;

struct dcp_job_cfg
{
    bool multiple_hits;
    bool hmmer3_compat;
};

DCP_API extern struct dcp_job_cfg const dcp_job_cfg_default;

struct dcp_job
{
    uint64_t id;
    char sid[DCP_JOB_SID_SIZE];
    struct dcp_job_cfg cfg;
    struct imm_abc const *abc;
    struct cco_queue seqs;
};

static inline void dcp_job_init(struct dcp_job *job, struct imm_abc const *abc)
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

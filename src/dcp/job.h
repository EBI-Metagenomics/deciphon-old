#ifndef DCP_JOB_H
#define DCP_JOB_H

#include "dcp/export.h"
#include "dcp/rc.h"
#include "dcp/target.h"
#include <stdbool.h>

struct dcp_job;
struct imm_abc;

struct dcp_job_cfg
{
    bool loglik;
    bool null;
    bool multiple_hits;
    bool hmmer3_compat;
};

DCP_API extern struct dcp_job_cfg const dcp_job_cfg_default;

struct dcp_job
{
    uint64_t id;
    struct dcp_job_cfg cfg;
    struct imm_abc const *abc;
    struct cco_queue targets;
};

static inline void dcp_job_init(struct dcp_job *job, struct imm_abc const *abc)
{
    job->cfg = dcp_job_cfg_default;
    job->abc = abc;
    cco_queue_init(&job->targets);
}

static inline void dcp_job_setup(struct dcp_job *job, struct dcp_job_cfg cfg)
{
    job->cfg = cfg;
}

DCP_API enum dcp_rc dcp_job_add(struct dcp_job *job, struct dcp_target *tgt);

#endif

#ifndef DCP_JOB_H
#define DCP_JOB_H

#include "cco/cco.h"
#include "dcp/job_state.h"
#include "dcp/seq.h"
#include <stdbool.h>

struct dcp_job
{
    bool multi_hits;
    bool hmmer3_compat;
    struct cco_queue seqs;
};

static inline void dcp_job_init(struct dcp_job *job, bool multi_hits,
                                bool hmmer3_compat)
{
    job->multi_hits = multi_hits;
    job->hmmer3_compat = hmmer3_compat;
    cco_queue_init(&job->seqs);
}

static inline void dcp_job_add_seq(struct dcp_job *job, struct dcp_seq *seq)
{
    cco_queue_put(&job->seqs, &seq->node);
}

#endif

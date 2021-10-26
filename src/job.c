#include "dcp/job.h"
#include "error.h"

struct dcp_job_cfg const dcp_job_cfg_default = {
    .loglik = true,
    .null = true,
    .multiple_hits = true,
    .hmmer3_compat = false,
};

enum dcp_rc dcp_job_add(struct dcp_job *job, struct dcp_target *tgt)
{
    if (tgt->seq.abc != job->abc)
        return error(DCP_ILLEGALARG, "different abc's");

    cco_queue_put(&job->targets, &tgt->node);
    return DCP_SUCCESS;
}

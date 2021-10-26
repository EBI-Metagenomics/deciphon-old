#include "dcp/job.h"
#include "error.h"

struct dcp_job_cfg const dcp_job_cfg_default = {
    .multiple_hits = true,
    .hmmer3_compat = false,
};

enum dcp_rc dcp_job_add(struct dcp_job *job, struct dcp_seq *seq)
{
    if (seq->seq.abc != job->abc)
        return error(DCP_ILLEGALARG, "different abc's");

    cco_queue_put(&job->seqs, &seq->node);
    return DCP_SUCCESS;
}

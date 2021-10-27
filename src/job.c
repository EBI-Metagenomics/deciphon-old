#include "job.h"
#include "error.h"

enum dcp_rc dcp_job_add(struct dcp_job *job, struct dcp_seq *seq)
{
    if (seq->seq.abc != job->abc->imm_abc)
        return error(DCP_ILLEGALARG, "different abc's");

    cco_queue_put(&job->seqs, &seq->node);
    return DCP_SUCCESS;
}

#include "dcp/task.h"
#include "error.h"

struct dcp_task_cfg const dcp_task_cfg_default = {
    .loglik = true,
    .null = true,
    .multiple_hits = true,
    .hmmer3_compat = false,
};

enum dcp_rc dcp_task_add(struct dcp_task *task, struct dcp_target *tgt)
{
    if (tgt->seq.abc != task->abc)
        return error(DCP_ILLEGALARG, "different abc's");

    cco_queue_put(&task->targets, &tgt->node);
    return DCP_SUCCESS;
}

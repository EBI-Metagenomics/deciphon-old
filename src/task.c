#include "task.h"
#include "logger.h"
#include "rc.h"

enum rc task_setup(struct task *task, struct imm_abc const *abc,
                       int64_t seq_id)
{
    enum rc rc = DONE;
    if ((rc = sched_seq_get(&task->sched_seq, seq_id))) return rc;

    struct imm_str str = imm_str(array_data(task->sched_seq.data));
    if (imm_abc_union_size(abc, str) > 0)
        return warn(ILLEGALARG, "out-of-alphabet characters");

    return rc;
}

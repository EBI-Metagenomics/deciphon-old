#include "scan_task.h"
#include "deciphon/errno.h"
#include "iseq.h"

void scan_task_init(struct scan_task *x) { x->task = NULL; }

int scan_task_setup(struct scan_task *x, struct imm_dp const *dp,
                    struct iseq const *seq)
{
  if (x->task && imm_task_reset(x->task, dp)) return DCP_EIMMRESETTASK;
  if (!x->task && !(x->task = imm_task_new(dp))) return DCP_EIMMNEWTASK;
  if (imm_task_setup(x->task, &seq->iseq)) return DCP_EIMMSETUPTASK;
  return 0;
}

void scan_task_cleanup(struct scan_task *x)
{
  if (x)
  {
    imm_task_del(x->task);
    x->task = NULL;
    imm_prod_cleanup(&x->prod);
  }
}

#ifndef SCAN_TASK_H
#define SCAN_TASK_H

#include "imm/imm.h"

struct iseq;

struct scan_task
{
  struct imm_task *task;
  struct imm_prod prod;
};

void scan_task_init(struct scan_task *);
int scan_task_setup(struct scan_task *, struct imm_dp const *,
                    struct iseq const *);
void scan_task_cleanup(struct scan_task *);

#endif

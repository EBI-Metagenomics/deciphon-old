#ifndef TASK_H
#define TASK_H

#include "prod.h"
#include "sched_job.h"
#include "sched_prod.h"
#include "sched_seq.h"

struct task
{
    struct sched_seq sched_seq;
    struct imm_seq seq;
    struct
    {
        struct imm_task *task;
        struct imm_prod prod;
    } alt;
    struct
    {
        struct imm_task *task;
        struct imm_prod prod;
    } null;
    struct prod prod;
};

enum rc task_setup(struct task *task, struct imm_abc const *abc,
                       int64_t seq_id);

#endif

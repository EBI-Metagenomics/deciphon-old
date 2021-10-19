#ifndef DCP_TASK_H
#define DCP_TASK_H

#include "dcp/export.h"
#include "dcp/rc.h"
#include "dcp/target.h"
#include <stdbool.h>

struct dcp_task;
struct imm_abc;

struct dcp_task_cfg
{
    bool loglik;
    bool null;
    bool multiple_hits;
    bool hmmer3_compat;
};

DCP_API extern struct dcp_task_cfg const dcp_task_cfg_default;

struct dcp_task
{
    uint64_t id;
    struct dcp_task_cfg cfg;
    struct imm_abc const *abc;
    struct cco_queue targets;
};

static inline void dcp_task_init(struct dcp_task *task,
                                 struct imm_abc const *abc)
{
    task->cfg = dcp_task_cfg_default;
    task->abc = abc;
    cco_queue_init(&task->targets);
}

static inline void dcp_task_setup(struct dcp_task *task,
                                  struct dcp_task_cfg cfg)
{
    task->cfg = cfg;
}

DCP_API enum dcp_rc dcp_task_add(struct dcp_task *task, struct dcp_target *tgt);

#endif

#ifndef DCP_TASK_H
#define DCP_TASK_H

#include "dcp/export.h"
#include "dcp/target.h"
#include <stdbool.h>

struct dcp_task;

struct dcp_task_cfg
{
    bool loglik;
    bool null;
    bool multiple_hits;
    bool hmmer3_compat;
};

struct dcp_task
{
    struct dcp_task_cfg cfg;
    struct cco_queue targets;
};

static inline void dcp_task_init(struct dcp_task *task, struct dcp_task_cfg cfg)
{
    task->cfg = cfg;
    cco_queue_init(&task->targets);
}

static inline void dcp_task_add(struct dcp_task *task, struct dcp_target *tgt)
{
    cco_queue_put(&task->targets, &tgt->node);
}

static inline struct dcp_task_cfg const *dcp_task_cfg(struct dcp_task const *t)
{
    return &t->cfg;
}

#endif

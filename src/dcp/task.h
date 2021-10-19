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

DCP_API void dcp_task_add_seq(struct dcp_task *task, char const *seq);
DCP_API struct dcp_task_cfg dcp_task_cfg(struct dcp_task *task);
DCP_API struct dcp_task *dcp_task_create(struct dcp_task_cfg cfg);
DCP_API void dcp_task_destroy(struct dcp_task *task);
DCP_API bool dcp_task_end(struct dcp_task *task);
DCP_API int dcp_task_errno(struct dcp_task const *task);

#endif

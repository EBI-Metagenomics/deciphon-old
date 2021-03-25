#ifndef DCP_TASK_H
#define DCP_TASK_H

#include "dcp/export.h"
#include <stdbool.h>

struct dcp_results;
struct dcp_task;

struct dcp_task_cfg
{
    bool loglik;
    bool null;
    bool multiple_hits;
    bool hmmer3_compat;
};

enum dcp_task_status
{
    TASK_STATUS_CREATED,
    TASK_STATUS_STOPPED,
    TASK_STATUS_FINISHED,
};

DCP_API void                 dcp_task_add_seq(struct dcp_task* task, char const* sequence);
DCP_API struct dcp_task*     dcp_task_create(struct dcp_task_cfg cfg);
DCP_API void                 dcp_task_destroy(struct dcp_task* task);
DCP_API bool                 dcp_task_eor(struct dcp_task* task);
DCP_API struct dcp_results*  dcp_task_read(struct dcp_task* task);
DCP_API enum dcp_task_status dcp_task_status(struct dcp_task const* task);

#endif

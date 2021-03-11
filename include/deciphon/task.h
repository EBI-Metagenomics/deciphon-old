#ifndef DECIPHON_TASK_H
#define DECIPHON_TASK_H

#include "deciphon/export.h"
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

DCP_API void                      dcp_task_add(struct dcp_task* task, char const* sequence);
DCP_API struct dcp_task*          dcp_task_create(struct dcp_task_cfg cfg);
DCP_API void                      dcp_task_destroy(struct dcp_task const* task);
DCP_API void                      dcp_task_reset(struct dcp_task* task);
DCP_API struct dcp_results const* dcp_task_results(struct dcp_task const* task);

#endif

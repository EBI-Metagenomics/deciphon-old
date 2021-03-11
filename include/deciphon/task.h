#ifndef DECIPHON_TASK_H
#define DECIPHON_TASK_H

#include "deciphon/export.h"
#include <stdbool.h>

struct dcp_results;
struct dcp_task;

DCP_API void                      dcp_task_add(struct dcp_task* task, char const* sequence);
DCP_API struct dcp_task*          dcp_task_create(bool loglik, bool null);
DCP_API void                      dcp_task_destroy(struct dcp_task const* task);
DCP_API void                      dcp_task_reset(struct dcp_task* task);
DCP_API struct dcp_results const* dcp_task_results(struct dcp_task const* task);

#endif

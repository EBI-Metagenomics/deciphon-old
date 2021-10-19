#include "dcp/task.h"

struct dcp_task_cfg const dcp_task_cfg_default = {
    .loglik = true,
    .null = true,
    .multiple_hits = true,
    .hmmer3_compat = false,
};

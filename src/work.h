#ifndef WORK_H
#define WORK_H

#include "dcp/job.h"
#include "imm/imm.h"
#include "path.h"
#include "sched.h"
#include <stdio.h>

struct prod_file
{
    PATH_TEMP_DECLARE(path);
    FILE *fd;
};

struct work
{
    struct dcp_job job;
    struct db *db;
    struct prod_file prod_file;
    struct imm_prod alt;
    struct imm_prod null;
};

#endif

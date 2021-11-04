#ifndef WORK_H
#define WORK_H

#include "dcp/job.h"
#include "imm/imm.h"
#include "path.h"
#include <stdio.h>

struct prod_file
{
    PATH_TEMP_DECLARE(path);
    FILE *fd;
};

struct db_handle;
struct db_pool;
struct sched;

struct work
{
    struct dcp_job job;
    struct db_handle *db;
    struct prod_file prod_file;
    struct imm_prod alt;
    struct imm_prod null;
};

enum dcp_rc work_fetch(struct work *, struct sched *, struct db_pool *);
enum dcp_rc work_run(struct work *work);

#endif

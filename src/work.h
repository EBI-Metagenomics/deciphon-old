#ifndef WORK_H
#define WORK_H

#include "dcp/job.h"
#include "path.h"
#include "imm/imm.h"
#include "prod_file.h"

struct db_handle;
struct db_pool;
struct sched;

struct work
{
    struct dcp_job job;
    char db_path[PATH_SIZE];
    struct db_handle *db;
    struct imm_abc const *abc;
    struct dcp_pro_prof const *prof;
    struct prod_file prod_file;
    struct imm_prod alt;
    struct imm_prod null;
};

enum dcp_rc work_fetch(struct work *, struct sched *, struct db_pool *);
enum dcp_rc work_run(struct work *work);

#endif

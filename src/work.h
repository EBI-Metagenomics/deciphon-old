#ifndef WORK_H
#define WORK_H

#include "dcp/job.h"
#include "dcp/limits.h"
#include "imm/imm.h"
#include "path.h"
#include "sched_job.h"
#include "sched_prod.h"
#include "sched_seq.h"
#include "xfile.h"
#include <stdatomic.h>
#include <stdint.h>

struct db_handle;
struct db_pool;
struct sched;

struct work_task
{
    struct sched_seq sched_seq;
    struct
    {
        struct imm_task *task;
        struct imm_prod prod;
    } alt;
    struct
    {
        struct imm_task *task;
        struct imm_prod prod;
    } null;
    struct dcp_prod prod;
};

struct work
{
    struct sched_job job;
    unsigned ntasks;
    struct work_task tasks[4];
    char db_path[DCP_PATH_SIZE];
    struct db_handle *db;
    struct dcp_pro_prof const *prof;
    struct xfile_tmp prod_file;
    atomic_bool failed;
};

void work_init(struct work *);
enum dcp_rc work_next(struct work *);
enum dcp_rc work_run(struct work *, unsigned num_threads);

#endif

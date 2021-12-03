#ifndef WORK_H
#define WORK_H

#include "dcp/job.h"
#include "dcp/limits.h"
#include "imm/imm.h"
#include "path.h"
#include "task.h"
#include "xfile.h"
#include <stdatomic.h>
#include <stdint.h>

#define WORK_MAX_NTASKS 64

struct db_handle;
struct tok;

struct work
{
    struct sched_job job;
    unsigned ntasks;
    struct task tasks[WORK_MAX_NTASKS];
    char db_path[DCP_PATH_SIZE];
    struct db_handle *db;
    struct dcp_pro_prof const *prof;
    struct xfile_tmp prod_file;
    atomic_bool failed;
    struct tok *tok;
};

void work_init(struct work *);
enum dcp_rc work_next(struct work *);
enum dcp_rc work_run(struct work *, unsigned num_threads);

#endif

#ifndef WORK_H
#define WORK_H

#include "dcp_limits.h"
#include "imm/imm.h"
#include "job.h"
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
    struct protein_profile const *prof;
    struct xfile_tmp prod_file;
    atomic_bool failed;
    struct tok *tok;
};

void work_init(struct work *);
enum rc work_next(struct work *);
enum rc work_run(struct work *, unsigned num_threads);

#endif

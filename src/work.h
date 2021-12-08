#ifndef WORK_H
#define WORK_H

#include "db.h"
#include "db_reader.h"
#include "dcp_limits.h"
#include "imm/imm.h"
#include "job.h"
#include "profile_reader.h"
#include "protein_db.h"
#include "standard_db.h"
#include "task.h"
#include "xfile.h"
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

#define WORK_MAX_NTASKS 64

struct tok;

struct work
{
    struct sched_job job;
    unsigned ntasks;
    struct task tasks[WORK_MAX_NTASKS];
    char db_path[DCP_PATH_SIZE];
    FILE *db_file;
    union
    {
        struct standard_db std;
        struct protein_db pro;
    } db;
    struct db_reader db_reader;
    struct profile_reader profile_reader;
    struct xfile_tmp prod_file;
    atomic_bool failed;
    struct tok *tok;
};

void work_init(struct work *);
enum rc work_next(struct work *);
enum rc work_run(struct work *, unsigned num_threads);

#endif

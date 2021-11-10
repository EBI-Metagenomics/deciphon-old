#ifndef WORK_H
#define WORK_H

#include "dcp/job.h"
#include "imm/imm.h"
#include "path.h"
#include "prod.h"
#include "prod_file.h"
#include "sched_job.h"
#include "sched_seq.h"
#include "xlimits.h"
#include <stdint.h>

struct db_handle;
struct db_pool;
struct sched;

struct work
{
    struct sched_job job;
    struct sched_seq seqs[128];
    unsigned nseqs;
    char db_path[PATH_SIZE];
    struct db_handle *db;
    struct dcp_pro_prof const *prof;
    struct
    {
        struct imm_task *alt;
        struct imm_task *null;
    } task;
    struct prod_file prod_file;
    struct
    {
        struct imm_prod alt;
        struct imm_prod null;
    } prod;
    struct prod prod_dcp;
};

void work_init(struct work *);
enum dcp_rc work_next(struct work *);
enum dcp_rc work_run(struct work *);

#endif

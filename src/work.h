#ifndef WORK_H
#define WORK_H

#include "dcp/job.h"
#include "imm/imm.h"
#include "path.h"
#include "prod.h"
#include "prod_file.h"
#include <stdint.h>

struct db_handle;
struct db_pool;
struct sched;

struct work
{
    struct sched *sched;
    struct dcp_job job;
    struct
    {
        int64_t id;
        struct dcp_seq dcp;
        struct imm_seq imm;
    } seq;
    char db_path[PATH_SIZE];
    struct db_handle *db;
    struct imm_abc const *abc;
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

void work_init(struct work *work, struct sched *sched);
enum dcp_rc work_fetch(struct work *, struct db_pool *);
enum dcp_rc work_run(struct work *work);

#endif

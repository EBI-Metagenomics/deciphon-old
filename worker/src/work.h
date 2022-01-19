#ifndef WORK_H
#define WORK_H

#include "common/rc.h"
#include "db.h"
#include "db_reader.h"
#include "hypothesis.h"
#include "imm/imm.h"
#include "job.h"
#include "prod.h"
#include "rest.h"
#include "profile_reader.h"
#include "protein_match.h"
#include "standard_match.h"
#include <stdint.h>

struct work
{
    struct sched_job job;
    struct sched_seq seq;
    struct prod prod[NUM_PARTITIONS];
    struct
    {
        struct db_reader reader;
        FILE *fp;
    } db;
    struct imm_str str;
    struct imm_seq iseq;
    struct imm_abc const *abc;
    char abc_name[6];
    struct profile_reader reader;
    enum profile_typeid profile_typeid;
    struct hypothesis null;
    struct hypothesis alt;

    prod_fwrite_match_cb *write_match_cb;
    union
    {
        struct standard_match std;
        struct protein_match pro;
    } match;

    imm_float lrt_threshold;
};

enum rc work_next(struct work *work);
enum rc work_run(struct work *work, unsigned num_threads);

#endif
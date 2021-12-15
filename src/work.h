#ifndef WORK_H
#define WORK_H

#include "db.h"
#include "db_reader.h"
#include "dcp_sched/sched.h"
#include "hypothesis.h"
#include "imm/imm.h"
#include "profile_reader.h"
#include "protein_match.h"
#include "rc.h"
#include "standard_match.h"
#include <stdint.h>

struct work
{
    struct sched_job job;
    struct sched_seq seq;
    struct sched_prod prod[DCP_NUM_PARTITIONS];
    struct
    {
        struct db_reader reader;
        FILE *fp;
    } db;
    struct imm_str str;
    struct imm_seq iseq;
    struct imm_abc const *abc;
    struct profile_reader reader;
    enum profile_typeid profile_typeid;
    struct hypothesis null;
    struct hypothesis alt;

    sched_prod_write_match_cb *write_match_cb;
    union
    {
        struct standard_match std;
        struct protein_match pro;
    } match;
};

enum rc work_next(struct work *work);
enum rc work_run(struct work *work, unsigned num_threads);

#endif

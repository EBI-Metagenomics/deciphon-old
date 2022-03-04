#ifndef DECIPHON_WORK_H
#define DECIPHON_WORK_H

#include "deciphon/db/db.h"
#include "deciphon/rc.h"
#include "deciphon/server/sched.h"
#include "hypothesis.h"
#include "imm/imm.h"
#include "prod.h"
#include "protein_match.h"
#include "sched.h"
#include <stdint.h>

struct work_priv
{
    struct prod prod;
    struct hypothesis null;
    struct hypothesis alt;
    union
    {
        // struct standard_match std;
        struct protein_match pro;
    } match;
};

struct work
{
    struct sched_job job;
    struct sched_seq seq;

    struct work_priv priv[NUM_PARTITIONS];
    struct db_reader db_reader;

    struct imm_str str;
    struct imm_seq iseq;
    struct imm_abc const *abc;
    char abc_name[6];
    struct profile_reader profile_reader;
    enum profile_typeid profile_typeid;

    prod_fwrite_match_cb *write_match_cb;
    imm_float lrt_threshold;
};

enum rc work_next(struct work *work);
enum rc work_run(struct work *work, unsigned num_threads);

#endif

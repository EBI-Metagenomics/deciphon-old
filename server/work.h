#ifndef WORK_H
#define WORK_H

#include "deciphon/db/db.h"
#include "deciphon/rc.h"
#include "deciphon/server/sched.h"
#include "imm/imm.h"
#include "sched.h"
#include "thread.h"
#include <stdint.h>

struct work
{
    struct sched_job job;
    struct sched_seq seq;

    unsigned num_threads;
    struct thread thread[NUM_PARTITIONS];

    char db_filename[FILENAME_SIZE];
    struct protein_db_reader db_reader;
    struct profile_reader profile_reader;

    struct imm_str str;
    struct imm_seq iseq;
    struct imm_abc const *abc;

    prod_fwrite_match_func_t *write_match_cb;
    imm_float lrt_threshold;
};

enum rc work_next(struct work *work);
enum rc work_prepare(struct work *work, unsigned num_threads);
enum rc work_run(struct work *work);

#endif

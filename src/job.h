#ifndef JOB_H
#define JOB_H

#include "cco/cco.h"
#include "job_state.h"
#include "seq.h"
#include <stdbool.h>

struct job
{
    int64_t id;
    int64_t db_id;
    bool multi_hits;
    bool hmmer3_compat;
    struct cco_queue seqs;
};

static inline void job_init(struct job *job, int64_t db_id)
{
    job->id = 0;
    job->db_id = db_id;
    job->multi_hits = true;
    job->hmmer3_compat = false;
    cco_queue_init(&job->seqs);
}

static inline void job_setup(struct job *job, bool multi_hits,
                             bool hmmer3_compat)
{
    job->multi_hits = multi_hits;
    job->hmmer3_compat = hmmer3_compat;
}

static inline void job_add_seq(struct job *job, struct seq *seq)
{
    cco_queue_put(&job->seqs, &seq->node);
}

#endif

#ifndef SCHED_SEQ_H
#define SCHED_SEQ_H

#include "cco/cco.h"

struct sched_seq
{
    uint64_t id;
    char const *data;
    uint64_t job_id;
    struct cco_node node;
};

static inline void sched_seq_init(struct sched_seq *seq, char const *data,
                                  uint64_t job_id)
{
    seq->id = 0;
    seq->data = data;
    seq->job_id = job_id;
    cco_node_init(&seq->node);
}

#endif

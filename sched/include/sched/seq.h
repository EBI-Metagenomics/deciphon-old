#ifndef DCP_SCHED_SEQ_H
#define DCP_SCHED_SEQ_H

#include "common/export.h"
#include "common/limits.h"
#include "common/rc.h"
#include <stdbool.h>
#include <stdint.h>

struct sched_seq
{
    int64_t id;
    int64_t job_id;
    char name[SEQ_NAME_SIZE];
    char data[SEQ_SIZE];
};

typedef void(sched_seq_set_cb)(struct sched_seq *seq, void *arg);
typedef bool(sched_seq_get_cb)(struct sched_seq **seq, void *arg);

EXPORT void sched_seq_init(struct sched_seq *seq, int64_t job_id,
                           char const *name, char const *data);
EXPORT enum rc sched_seq_next(struct sched_seq *seq);

#endif

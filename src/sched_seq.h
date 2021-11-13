#ifndef SCHED_SEQ_H
#define SCHED_SEQ_H

#include "dcp/limits.h"
#include <stdint.h>

struct sqlite3;

struct sched_seq
{
    int64_t id;
    int64_t job_id;
    char name[DCP_SEQ_NAME_SIZE];
    char data[DCP_MATCH_DATA_SIZE];
};

#define SCHED_SEQ_INIT(job_id)                                                 \
    {                                                                          \
        0, job_id, "", ""                                                      \
    }

enum dcp_rc sched_seq_module_init(struct sqlite3 *db);
void sched_seq_setup(struct sched_seq *seq, char const name[DCP_SEQ_NAME_SIZE],
                     char const data[DCP_MATCH_DATA_SIZE]);
enum dcp_rc sched_seq_add(struct sched_seq *seq);
enum dcp_rc sched_seq_next(int64_t job_id, int64_t *seq_id);
enum dcp_rc sched_seq_get(struct sched_seq *seq, int64_t id);
void sched_seq_module_del(void);

#endif

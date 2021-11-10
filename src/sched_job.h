#ifndef SCHED_JOB_H
#define SCHED_JOB_H

#include "dcp/job_state.h"
#include "sched_limits.h"
#include <stdint.h>

struct sqlite3;

struct sched_job
{
    int64_t id;

    int64_t db_id;
    int32_t multi_hits;
    int32_t hmmer3_compat;
    char state[SCHED_STATE_SIZE];

    char error[SCHED_ERROR_SIZE];
    int64_t submission;
    int64_t exec_started;
    int64_t exec_ended;
};

#define SCHED_JOB_INIT(db_id, multi_hits, hmmer3_compat, submission)           \
    {                                                                          \
        0, db_id, multi_hits, hmmer3_compat, "", "", submission, 0, 0          \
    }

enum dcp_rc sched_job_module_init(struct sqlite3 *db);
enum dcp_rc sched_job_add(struct sched_job *job);
enum dcp_rc sched_job_state(int64_t job_id, enum dcp_job_state *state);
enum dcp_rc sched_job_next_pending(int64_t *job_id);
enum dcp_rc sched_job_get(struct sched_job *job, int64_t job_id);
void sched_job_module_del(void);

#endif

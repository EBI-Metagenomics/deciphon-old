#ifndef SCHED_JOB_H
#define SCHED_JOB_H

#include "dcp_limits.h"
#include "job_state.h"
#include "sched_limits.h"
#include <stdint.h>

struct sched_job
{
    int64_t id;

    int64_t db_id;
    int32_t multi_hits;
    int32_t hmmer3_compat;
    char state[SCHED_JOB_STATE_SIZE];

    char error[SCHED_JOB_ERROR_SIZE];
    int64_t submission;
    int64_t exec_started;
    int64_t exec_ended;
};

#define SCHED_JOB_INIT(db_id, multi_hits, hmmer3_compat, submission)           \
    {                                                                          \
        0, db_id, multi_hits, hmmer3_compat, "pend", "", submission, 0, 0      \
    }

enum rc sched_job_module_init(void);
enum rc sched_job_add(struct sched_job *job);
enum rc sched_job_state(int64_t job_id, enum job_state *state);
enum rc sched_job_next_pending(int64_t *job_id);
enum rc sched_job_set_error(int64_t job_id, char const *error,
                            int64_t exec_ended);
enum rc sched_job_set_done(int64_t job_id, int64_t exec_ended);
enum rc sched_job_get(struct sched_job *job, int64_t job_id);
void sched_job_module_del(void);

#endif

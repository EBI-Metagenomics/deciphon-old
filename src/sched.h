#ifndef SCHED_H
#define SCHED_H

#include "xlimits.h"
#include <stdint.h>

struct dcp_job;
struct sqlite3;

struct sched
{
    struct sqlite3 *db;
};

enum dcp_rc sched_setup(char const filepath[PATH_SIZE]);
enum dcp_rc sched_open(struct sched *sched, char const filepath[PATH_SIZE]);
enum dcp_rc sched_close(struct sched *sched);
enum dcp_rc sched_submit_job(struct sched *sched, struct dcp_job *job,
                             int64_t db_id, int64_t *job_id);
enum dcp_rc sched_fetch_pend_job(struct sched *sched, struct dcp_job *job,
                                 int64_t db_id, int64_t *job_id);

#endif

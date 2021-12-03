#ifndef SCHED_H
#define SCHED_H

#include "dcp_limits.h"
#include <stdint.h>

struct dcp_job;
struct sqlite3;

enum rc sched_setup(char const filepath[DCP_PATH_SIZE]);
enum rc sched_open(char const filepath[DCP_PATH_SIZE]);
enum rc sched_close(void);
struct sqlite3 *sched_db(void);
enum rc sched_submit_job(struct dcp_job *job);
enum rc sched_fetch_pend_job(struct dcp_job *job, int64_t db_id,
                                 int64_t *job_id);

#endif

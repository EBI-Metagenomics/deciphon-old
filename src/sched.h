#ifndef SCHED_H
#define SCHED_H

#include "xlimits.h"
#include <stdint.h>

struct dcp_job;
struct sqlite3;

enum dcp_rc sched_setup(char const filepath[PATH_SIZE]);
enum dcp_rc sched_open(char const filepath[PATH_SIZE]);
enum dcp_rc sched_close(void);
struct sqlite3 *sched_db(void);
enum dcp_rc sched_submit_job(struct dcp_job *job);
enum dcp_rc sched_fetch_pend_job(struct dcp_job *job, int64_t db_id,
                                 int64_t *job_id);

#endif

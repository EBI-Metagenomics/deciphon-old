#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>

struct job;
struct sqlite3;

extern struct sqlite3 *sched;

enum rc sched_setup(char const *filepath);
enum rc sched_open(char const *filepath);
enum rc sched_close(void);
enum rc sched_submit_job(struct job *job);
enum rc sched_fetch_pend_job(struct job *job, int64_t db_id, int64_t *job_id);

#endif

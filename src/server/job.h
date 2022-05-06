#ifndef JOB_H
#define JOB_H

#include "deciphon/sched/sched.h"

struct job
{
    struct sched_job sched;
    unsigned num_threads;
};

void job_init(struct job *, unsigned num_threads);
enum rc job_next(struct job *);
enum rc job_run(struct job *);
void job_set_fail(int64_t job_id, char const *fmt, ...);

#endif

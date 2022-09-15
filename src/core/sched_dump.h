#ifndef CORE_SCHED_DUMP_H
#define CORE_SCHED_DUMP_H

struct sched_db;
struct sched_hmm;
struct sched_job;
struct sched_scan;

char *sched_dump_db(struct sched_db const *, char buffer[]);
char *sched_dump_hmm(struct sched_hmm const *, char buffer[]);
char *sched_dump_job(struct sched_job const *, char buffer[]);
char *sched_dump_scan(struct sched_scan const *, char buffer[]);

#endif

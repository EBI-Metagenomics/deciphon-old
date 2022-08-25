#ifndef DECIPHON_SCHED_DUMP_H
#define DECIPHON_SCHED_DUMP_H

struct sched_db;
struct sched_hmm;
struct sched_job;

char *sched_dump_hmm(struct sched_hmm const *, unsigned size, char *buffer);
char *sched_dump_db(struct sched_db const *, unsigned size, char *buffer);
char *sched_dump_job(struct sched_job const *, unsigned size, char *buffer);

#endif

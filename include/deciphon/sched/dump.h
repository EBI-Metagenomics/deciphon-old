#ifndef DECIPHON_SCHED_DUMP_H
#define DECIPHON_SCHED_DUMP_H

struct sched_db;
struct sched_hmm;

char *sched_dump_hmm(struct sched_hmm const *, unsigned size, char *buffer);
char *sched_dump_db(struct sched_db const *, unsigned size, char *buffer);

#endif

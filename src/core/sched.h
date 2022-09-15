#ifndef CORE_SCHED_H
#define CORE_SCHED_H

#include "sched_structs.h"
#include <stdbool.h>
#include <stdint.h>

struct jr;

enum sched_job_type sched_job_type(struct sched_job const *job);

void sched_db_init(struct sched_db *);
void sched_hmm_init(struct sched_hmm *);
void sched_job_init(struct sched_job *);
void sched_scan_init(struct sched_scan *);
void sched_seq_init(struct sched_seq *);

enum rc sched_db_parse(struct sched_db *, struct jr *);
enum rc sched_hmm_parse(struct sched_hmm *, struct jr *);
enum rc sched_job_parse(struct sched_job *, struct jr *);
enum rc sched_scan_parse(struct sched_scan *s, struct jr *);
enum rc sched_seq_parse(struct sched_seq *, struct jr *);

#endif

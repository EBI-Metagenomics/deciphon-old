#ifndef DECIPHON_SCHED_SCHED_H
#define DECIPHON_SCHED_SCHED_H

#include "sched/structs.h"
#include <stdbool.h>
#include <stdint.h>

static inline enum sched_job_type sched_job_type(struct sched_job const *job)
{
    return (enum sched_job_type)job->type;
}

struct xjson;
struct jr;

void sched_db_init(struct sched_db *);
void sched_hmm_init(struct sched_hmm *);
void sched_job_init(struct sched_job *);
void sched_scan_init(struct sched_scan *);
void sched_seq_init(struct sched_seq *);

enum rc sched_db_parse(struct sched_db *, struct jr *);
enum rc sched_hmm_parse(struct sched_hmm *, struct jr *);
enum rc sched_job_parse(struct sched_job *, struct jr *);
enum rc sched_scan_parse(struct sched_scan *, struct xjson *x, unsigned start);
enum rc sched_seq_parse(struct sched_seq *, struct jr *);

#endif

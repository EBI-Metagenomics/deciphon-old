#include "sched_job.h"
#include "error.h"

void sched_job_setup(struct sched_job *job, bool multiple_hits,
                     bool hmmer3_compat, uint64_t db_id)
{
    job->id = 0;
    job->multiple_hits = multiple_hits;
    job->hmmer3_compat = hmmer3_compat;
    job->db_id = db_id;
    job->state = SCHED_JOB_PEND;
    job->error[0] = '\0';
    job->submission = DCP_UTC_NA;
    job->exec_started = DCP_UTC_NA;
    job->exec_ended = DCP_UTC_NA;
    cco_queue_init(&job->seqs);
}

void sched_job_add_seq(struct sched_job *job, struct sched_seq *seq)
{
    cco_queue_put(&job->seqs, &seq->node);
}

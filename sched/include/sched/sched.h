#ifndef SCHED_SCHED_H
#define SCHED_SCHED_H

#include "common/export.h"
#include "common/rc.h"
#include "sched/prod.h"
#include "sched/seq.h"
#include <stdint.h>

struct sched_db;
struct sched_job;
struct sched_prod;
struct sched_seq;

EXPORT enum rc sched_setup(char const *filepath);
EXPORT enum rc sched_open(void);
EXPORT enum rc sched_close(void);

EXPORT enum rc sched_add_db(char const *filepath, int64_t *id);
EXPORT enum rc sched_cpy_db_filepath(unsigned size, char *filepath, int64_t id);
EXPORT enum rc sched_get_job(struct sched_job *job);
EXPORT enum rc sched_get_seq(struct sched_seq *seq);
EXPORT enum rc sched_get_db(struct sched_db *db);
EXPORT enum rc sched_get_prod(struct sched_prod *prod);

EXPORT enum rc sched_get_job_prods(int64_t job_id, sched_prod_set_cb cb,
                                   struct sched_prod *prod, void *arg);

EXPORT enum rc sched_get_job_seqs(int64_t job_id, sched_seq_set_cb cb,
                                  struct sched_seq *seq, void *arg);

EXPORT enum rc sched_set_job_fail(int64_t job_id, char const *msg);
EXPORT enum rc sched_set_job_done(int64_t job_id);

EXPORT enum rc sched_submit_job(struct sched_job *job, char const *filepath,
                                char *error);
EXPORT enum rc sched_submit_job2(struct sched_job *job, sched_seq_get_cb cb,
                                 void *arg);

EXPORT enum rc sched_begin_job_submission(struct sched_job *job);
EXPORT void sched_add_seq(struct sched_job *job, char const *name,
                          char const *data);
EXPORT enum rc sched_rollback_job_submission(struct sched_job *job);
EXPORT enum rc sched_end_job_submission(struct sched_job *job);

EXPORT enum rc sched_begin_prod_submission(unsigned num_threads);
EXPORT enum rc sched_end_prod_submission(void);

EXPORT enum rc sched_next_pending_job(struct sched_job *job);

#endif

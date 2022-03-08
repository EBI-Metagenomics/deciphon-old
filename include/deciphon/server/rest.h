#ifndef DECIPHON_SERVER_REST_H
#define DECIPHON_SERVER_REST_H

#include "deciphon/limits.h"
#include "deciphon/rc.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

struct rest_job_state
{
    enum rc rc;
    char error[JOB_ERROR_SIZE];
    char state[JOB_STATE_SIZE];
};

struct rest_pend_job
{
    int64_t id;
    int64_t db_id;
    bool multi_hits;
    bool hmmer3_compat;
};

struct rest_ret
{
    enum rc rc;
    char error[ERROR_SIZE];
};

extern struct rest_ret rest_ret;
extern struct rest_job_state job_state;

enum sched_job_state;
struct sched_db;
struct sched_job;
struct sched_seq;

enum rc rest_open(char const *url_stem);
void rest_close(void);
enum rc rest_wipe(void);
enum rc rest_next_pend_job(struct sched_job *job);
enum rc rest_post_db(struct sched_db *db);
enum rc rest_get_db(struct sched_db *db);
// enum rc rest_set_job_state(struct sched_job *job, enum sched_job_state state,
//                            char const *error);
// enum rc rest_get_db(struct sched_db *db);
// enum rc rest_next_seq(struct sched_seq *seq);
// enum rc rest_submit_prods_file(char const *filepath);

#endif
#ifndef SCHED_H
#define SCHED_H

#include "dcp/job_state.h"
#include "dcp/rc.h"
#include <sqlite3.h>
#include <stdint.h>

struct dcp_job;

struct sched
{
    sqlite3 *db;
    struct
    {
        sqlite3_stmt *begin;
        struct
        {
            sqlite3_stmt *job;
            sqlite3_stmt *seq;
        } submit;
        struct
        {
            sqlite3_stmt *state;
        } job;
        sqlite3_stmt *end;
    } stmt;
};

enum dcp_rc sched_setup(char const *filepath);
enum dcp_rc sched_open(struct sched *sched, char const *filepath);
enum dcp_rc sched_close(struct sched *sched);
enum dcp_rc sched_submit_job(struct sched *sched, struct dcp_job *job,
                             uint64_t db_id, uint64_t *job_id);
enum dcp_rc sched_add_db(struct sched *sched, char const *filepath,
                         uint64_t *id);
enum dcp_rc sched_job_state(struct sched *, uint64_t, enum dcp_job_state *);

#endif

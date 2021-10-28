#ifndef SCHED_H
#define SCHED_H

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
        sqlite3_stmt *end;
    } stmt;
};

enum dcp_rc sched_setup(char const *filepath);
enum dcp_rc sched_open(struct sched *sched, char const *filepath);
enum dcp_rc sched_close(struct sched *sched);
enum dcp_rc sched_submit(struct sched *sched, struct dcp_job *job,
                         uint64_t db_id);
enum dcp_rc sched_add_db(struct sched *sched, char const *filepath,
                         uint64_t *id);

#endif

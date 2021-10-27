#ifndef SCHED_H
#define SCHED_H

#include "dcp/rc.h"
#include <sqlite3.h>

struct sched_job;

struct sched
{
    sqlite3 *db;
};

enum dcp_rc sched_setup(char const *filepath);
enum dcp_rc sched_open(struct sched *sched, char const *filepath);
enum dcp_rc sched_close(struct sched *sched);
enum dcp_rc sched_submit(struct sched *sched, struct sched_job *job);
enum dcp_rc sched_add_db(struct sched *sched, char const *filepath);

#endif

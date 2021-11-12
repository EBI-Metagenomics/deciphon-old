#ifndef SCHED_DB_H
#define SCHED_DB_H

#include "sched_limits.h"
#include "dcp/limits.h"
#include <stdint.h>

struct sqlite3;

struct sched_db
{
    int64_t id;
    char name[DCP_DB_NAME_SIZE];
    char filepath[PATH_SIZE];
};

#define SCHED_DB_INIT()                                                        \
    {                                                                          \
        0, "", ""                                                              \
    }

void sched_db_setup(struct sched_db *db, char const name[DCP_DB_NAME_SIZE],
                    char const filepath[PATH_SIZE]);
enum dcp_rc sched_db_module_init(struct sqlite3 *db);
enum dcp_rc sched_db_add(struct sched_db *db);
enum dcp_rc sched_db_get(struct sched_db *db, int64_t db_id);
enum dcp_rc sched_db_exist(struct sched_db *db, char const name[DCP_DB_NAME_SIZE]);
void sched_db_module_del(void);

#endif

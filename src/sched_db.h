#ifndef SCHED_DB_H
#define SCHED_DB_H

#include "dcp_limits.h"
#include <stdint.h>

struct sched_db
{
    int64_t id;
    int64_t xxh64;
    char name[DCP_DB_NAME_SIZE];
    char filepath[DCP_PATH_SIZE];
};

enum rc sched_db_setup(struct sched_db *db, char const *name,
                       char const *filepath);
enum rc sched_db_module_init(void);
enum rc sched_db_add(struct sched_db *db);
enum rc sched_db_get_by_id(struct sched_db *db, int64_t id);
enum rc sched_db_get_by_xxh64(struct sched_db *db, int64_t xxh64);

void sched_db_module_del(void);

#endif

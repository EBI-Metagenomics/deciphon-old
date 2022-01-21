#ifndef SCHED_DB_H
#define SCHED_DB_H

#include "common/export.h"
#include "common/limits.h"
#include <stdint.h>

struct sched_db
{
    int64_t id;
    int64_t xxh64;
    char filename[FILENAME_SIZE];
};

typedef void(sched_db_set_cb)(struct sched_db *db, void *arg);

EXPORT void sched_db_init(struct sched_db *db);
EXPORT enum rc sched_db_get(struct sched_db *db);
EXPORT enum rc sched_db_add(struct sched_db *db, char const *filename);
EXPORT enum rc sched_db_get_all(sched_db_set_cb cb, struct sched_db *db,
                                void *arg);

#endif

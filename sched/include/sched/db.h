#ifndef SCHED_DB_H
#define SCHED_DB_H

#include "common/export.h"
#include "common/limits.h"
#include <stdint.h>

struct sched_db
{
    int64_t id;
    int64_t xxh64;
    char filepath[PATH_SIZE];
};

// struct sched_db
// {
//     int64_t id;
//     char name[FILENAME_SIZE];
// };
//
// typedef void sched_db_peek_t(struct sched_db const *db, void *arg);
//
// EXPORT enum rc sched_db_list(sched_db_peek_t *peek, void *arg);

#endif

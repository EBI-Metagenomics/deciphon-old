#ifndef DB_H
#define DB_H

#include "common/limits.h"
#include "sched/db.h"
#include <stdint.h>

enum rc db_has(char const *filename, struct sched_db *db);
enum rc db_hash(char const *filename, int64_t *xxh64);
enum rc db_get_by_id(struct sched_db *db, int64_t id);
enum rc db_get_by_xxh64(struct sched_db *db, int64_t xxh64);

#endif

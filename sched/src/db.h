#ifndef DB_H
#define DB_H

#include "common/limits.h"
#include "sched/db.h"
#include <stdint.h>

enum rc db_add(char const *filepath, int64_t *id);
enum rc db_has(char const *filepath, struct sched_db *db);
enum rc db_hash(char const *filepath, int64_t *xxh64);
enum rc db_get_by_id(struct sched_db *db, int64_t id);
enum rc db_get_by_xxh64(struct sched_db *db, int64_t xxh64);
void db_name(char *filename, char const *path);

#endif

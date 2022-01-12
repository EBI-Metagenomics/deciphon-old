#ifndef DB_H
#define DB_H

#include "common/limits.h"
#include <stdint.h>

struct db
{
    int64_t id;
    int64_t xxh64;
    char filepath[PATH_SIZE];
};

enum rc db_add(char const *filepath, int64_t *id);
enum rc db_has(char const *filepath, struct db *db);
enum rc db_hash(char const *filepath, int64_t *xxh64);
enum rc db_get_by_id(struct db *db, int64_t id);
enum rc db_get_by_xxh64(struct db *db, int64_t xxh64);

#endif

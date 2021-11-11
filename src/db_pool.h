#ifndef DB_POOL_H
#define DB_POOL_H

#include <stdint.h>

struct db_handle;

void db_pool_module_init(void);
struct db_handle *db_pool_new(int64_t id);
struct db_handle *db_pool_get(int64_t id);
void db_pool_del(struct db_handle *db);
struct db_handle *db_pool_fetch(int64_t id);

#endif

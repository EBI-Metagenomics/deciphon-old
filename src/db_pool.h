#ifndef DB_POOL_H
#define DB_POOL_H

#include "cco/cco.h"
#include "db_handle.h"
#include "ilog2.h"
#include "pool.h"

struct db_pool
{
    struct db_handle db_handles[POOL_SIZE];
    CCO_HASH_DECLARE(handle_map, ilog2(POOL_SIZE));
    struct pool pool;
};

void db_pool_init(struct db_pool *pool);
struct db_handle *db_pool_new(struct db_pool *pool, dcp_sched_id id);
struct db_handle *db_pool_get(struct db_pool *pool, dcp_sched_id id);
void db_pool_del(struct db_pool *pool, struct cco_hnode *node);

#endif

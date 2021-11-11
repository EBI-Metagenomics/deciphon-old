#include "db_pool.h"
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

static struct db_pool pool = {0};

void db_pool_module_init(void)
{
    pool_init(&pool.pool);
    cco_hash_init(pool.handle_map);
    for (unsigned i = 0; i < POOL_SIZE; ++i)
        pool_assoc(&pool.pool, &pool.db_handles[i].pool_id);
}

struct db_handle *db_pool_new(int64_t id)
{
    unsigned *pool_id = pool_pop(&pool.pool);
    struct db_handle *db = cco_of(pool_id, struct db_handle, pool_id);
    db_handle_init(db, id);
    cco_hash_add(pool.handle_map, &db->hnode, id);
    return db;
}

struct db_handle *db_pool_get(int64_t id)
{
    struct db_handle *db = NULL;
    cco_hash_for_each_possible(pool.handle_map, db, hnode, id)
    {
        if (db->id == id) break;
    }
    return db;
}

void db_pool_del(struct db_handle *db) { cco_hash_del(&db->hnode); }

struct db_handle *db_pool_fetch(int64_t id)
{
    struct db_handle *db = db_pool_get(id);
    if (!db) db = db_pool_new(id);
    return db;
}

#include "db_pool.h"

void db_pool_init(struct db_pool *pool)
{
    pool_init(&pool->pool);
    cco_hash_init(pool->handle_map);
    for (unsigned i = 0; i < POOL_SIZE; ++i)
        pool_assoc(&pool->pool, &pool->db_handles[i].pool_id);
}

struct db_handle *db_pool_new(struct db_pool *pool, int64_t id)
{
    unsigned *pool_id = pool_pop(&pool->pool);
    struct db_handle *db = cco_of(pool_id, struct db_handle, pool_id);
    cco_hnode_init(&db->hnode);
    cco_hash_add(pool->handle_map, &db->hnode, id);
    db->sched_id = id;
    return db;
}

struct db_handle *db_pool_get(struct db_pool *pool, int64_t id)
{
    struct db_handle *db = NULL;
    cco_hash_for_each_possible(pool->handle_map, db, hnode, id)
    {
        if (db->sched_id == id) break;
    }
    return db;
}

void db_pool_del(struct db_pool *pool, struct cco_hnode *node)
{
    cco_hash_del(node);
}

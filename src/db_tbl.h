#ifndef DB_TBL_H
#define DB_TBL_H

#include "dnode.h"
#include "cco/cco.h"
#include "dcp/db.h"
#include "dcp/sched.h"
#include "dcp/pro_db.h"
#include "dcp/std_db.h"
#include "deck.h"

struct db
{
    dcp_sched_id id;
    struct dnode dnode;
    struct cco_hnode hnode;
    FILE *fd;
    union
    {
        struct dcp_std_db std;
        struct dcp_pro_db pro;
    };
};

struct db_tbl
{
    struct db dbs[32];
    struct deck deck;
    CCO_HASH_DECLARE(tbl, 6);
};

static void db_tbl_init(struct db_tbl *tbl)
{
    deck_init(&tbl->deck);
    cco_hash_init(tbl->tbl);
    for (unsigned i = 0; i < 32; ++i)
        deck_assoc(&tbl->deck, &tbl->dbs[i].dnode);
}

static struct db *db_tbl_new(struct db_tbl *tbl, dcp_sched_id id)
{
    struct dnode *dnode = deck_pop(&tbl->deck);
    struct db *db = cco_of(dnode, struct db, dnode);
    cco_hnode_init(&db->hnode);
    cco_hash_add(tbl->tbl, &db->hnode, id);
    db->id = id;
    return db;
}

static struct db *db_tbl_get(struct db_tbl *tbl, dcp_sched_id id)
{
    struct db *db = NULL;
    cco_hash_for_each_possible(tbl->tbl, db, hnode, id)
    {
        if (db->id == id) break;
    }
    return db;
}

static void db_tbl_del(struct db_tbl *tbl, struct cco_hnode *node)
{
    cco_hash_del(node);
}

#endif

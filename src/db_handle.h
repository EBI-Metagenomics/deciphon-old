#ifndef DB_HANDLE_H
#define DB_HANDLE_H

#include "cco/cco.h"
#include "dcp/pro_db.h"
#include "dcp/rc.h"
#include "dcp/std_db.h"
#include "path.h"
#include "xlimits.h"
#include <stdio.h>

struct sched;

struct db_handle
{
    int64_t id;
    unsigned pool_id;
    struct cco_hnode hnode;
    FILE *fd;
    uint64_t open_since;
    union
    {
        struct dcp_std_db std;
        struct dcp_pro_db pro;
    };
};

void db_handle_init(struct db_handle *db, int64_t id);
enum dcp_rc db_handle_open(struct db_handle *db, char path[PATH_SIZE]);
enum dcp_rc db_handle_close(struct db_handle *db);

static inline bool db_handle_is_open(struct db_handle const *db)
{
    return db->open_since != 0;
}

#endif

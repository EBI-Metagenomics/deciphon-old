#ifndef DB_HANDLE_H
#define DB_HANDLE_H

#include "cco/cco.h"
#include "dcp/limits.h"
#include "dcp/pro_db.h"
#include "dcp/rc.h"
#include "dcp/std_db.h"
#include "path.h"
#include <stdio.h>

#define DB_HANDLE_MAX_FILES 64

struct db_handle
{
    int64_t id;
    unsigned pool_id;
    struct cco_hnode hnode;
    int nfiles;
    FILE *fp[DB_HANDLE_MAX_FILES];
    uint64_t open_since;
    union
    {
        struct dcp_std_db std;
        struct dcp_pro_db pro;
    };
};

void db_handle_init(struct db_handle *db, int64_t id);
enum dcp_rc db_handle_open(struct db_handle *db, char path[DCP_PATH_SIZE],
                           int nfiles);
enum dcp_rc db_handle_close(struct db_handle *db);

#endif
